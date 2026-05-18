#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <assert.h>
#include <time.h>

//Talvez esteja faltando libavformat
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "types.h"
#include "status.h"
#include "sockets.h"
#include "rtp.h"
#include "server.h"

#include "program.cpp"
#include "rtp.cpp"

char* fileToString(FILE* file){
    uint64 startingPos = ftell(file);
    fseek(file, 0, SEEK_END);  
    uint64 size = ftell(file); 
    fseek(file, startingPos, SEEK_SET);  

    char* string = (char*)malloc(size + 1);
    if(!string){
        printf("Não houve memória o suficiente para alocar %lu bytes de memória.\n", size);
        return NULL;
    }

    char* buffer = string;
    while(fgets(buffer, size, file)){
        buffer += strlen(buffer);
    }

    return string;
}

char* fileToString(const char* path){
    FILE* file = fopen(path, "r");
    if(!file){
        printf("Error opening file.");
        return NULL; 
    }

    char* string = fileToString(file);
    fclose(file);

    return string;
}

void printMemory(void* memoryArg, size_t size){
    uint8* memory = (uint8*)memoryArg;
    for(int i = 0;
            i < size;
            i++)
    {
        uint8* currentByte = &memory[i];

        for(int i = 7;
                i >= 0;
                i--)
        {
            if(*currentByte & 1 << i){
                printf("1");
            }else{
                printf("0");
            }
        }

        printf(" ");
    }
}

void endiannesTest() {
    uint16 normalValue = 9;
    printMemory(&normalValue, sizeof(normalValue));
    
    printf("\n");

    normalValue = htons(normalValue);
    printMemory(&normalValue, sizeof(normalValue));
    printf("\n");
}

int main(){
    /*
    rfc 3550
    2.2 Audio and Video Conference

   If both audio and video media are used in a conference, they are
   transmitted as separate RTP sessions.  That is, separate RTP and RTCP
   packets are transmitted for each medium using two different UDP port
   pairs and/or multicast addresses.  There is no direct coupling at the
   RTP level between the audio and video sessions, except that a user
   participating in both sessions should use the same distinguished
   (canonical) name in the RTCP packets for both so that the sessions
   can be associated.

   One motivation for this separation is to allow some participants in
   the conference to receive only one medium if they choose.  Further
   explanation is given in Section 5.2.  Despite the separation,
   synchronized playback of a source's audio and video can be achieved
   using timing information carried in the RTCP packets for both
   sessions.
   */

    const uint32 serverSsrc = random();
    RtpStream videoStream;
    {
        FunctionReturnStatus status = videoStream.init(serverVideoStreamDataPort);
        if(status.error > 0){
            printf("Erro ao criar stream de vídeo por -> %s\n", status.message);
            return 0;
        }
        videoStream.ssrc = serverSsrc;
    }
    printf("Stream de vídeo criada.\n");
    printf("Porta do socket de dados: %d.\n", videoStream.dataSocket.fd);
    printf("Porta do socket de controle: %d.\n", videoStream.controlSocket.fd);
    printf("\n");

    //Parte secundária
    RtpStream audioStream;
    {
        FunctionReturnStatus status = audioStream.init(serverAudioStreamDataPort);
        if(status.error > 0){
            printf("Erro ao criar stream de áudio por -> %s.\n", status.message);
            return 0;
        }
        audioStream.ssrc = serverSsrc + 1;
    }
    printf("Stream de áudio criada.\n");
    printf("Porta do socket de dados: %d.\n", audioStream.dataSocket.fd);
    printf("Porta do socket de controle: %d.\n", audioStream.controlSocket.fd);
    printf("\n");

    Display *display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);
    XWindowAttributes gwa;

    XGetWindowAttributes(display, root, &gwa);
    int width = gwa.width;
    int height = gwa.height;

    struct SwsContext *swsContext = sws_getContext(
            width, height, AV_PIX_FMT_BGRA,          // Source: X11 format
            width, height, AV_PIX_FMT_YUV420P,        // Destination: H264 format
            SWS_BICUBIC, NULL, NULL, NULL
            );

    Window win = XCreateSimpleWindow(display, root, 0, 0, width/2, height/2, 1, 0, 0);
    XMapWindow(display, win);
    GC gc = XCreateGC(display, win, 0, NULL);

    AVFrame *pFrame = av_frame_alloc();
    enum AVPixelFormat pixelFormat = AV_PIX_FMT_YUV420P;
    pFrame->format = pixelFormat;
    pFrame->width  = width;
    pFrame->height = height;
    const AVCodec* h264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!h264Codec){
        printf("Não há o codec para h264.\n");
        return 1;
    }

    AVCodecContext* avCodecContext = avcodec_alloc_context3(h264Codec);
    avCodecContext->bit_rate = 400000;         
    avCodecContext->width = width;            
    avCodecContext->height = height;
    avCodecContext->time_base = (AVRational){1, 60};
    avCodecContext->framerate = (AVRational){0, 60};
    avCodecContext->gop_size = 10;            
    avCodecContext->max_b_frames = 1;
    avCodecContext->pix_fmt = pixelFormat; 

    if (avcodec_open2(avCodecContext, h264Codec, NULL) < 0) {
        printf("Não foi possível abrir o codec.\n");
        return 1;
    }

    RtpStream clients[maximumAmountOfConnections];
    int clientCount = 0;

    AVPacket* framePacket = av_packet_alloc();

    av_frame_get_buffer(pFrame, 0);

    int frameCount = 0;
    time_t startingTime = time(NULL);
    const uint32 timeStampOffset = random();
    uint32 currentTimeStamp = timeStampOffset; 
    while(1){
        if(clientCount == 0){
            printf("Servidor escutando na porta %d", videoStream.dataSocket.port);
            socketSinglePoll(videoStream.dataSocket.fd, POLLIN, 300);
        }
        //Pegar imagem da tela
        XImage *image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);

        frameCount++;
        pFrame->pts = frameCount;

        //Converter para YUV
        //Ver se eu posso fazer esse typecasting em data
        const uint8 *inData[1] = { (uint8 *)image->data };
        int inLinesize[1] = { image->bytes_per_line };
        sws_scale(swsContext, 
                inData,           // Conteúdo da imagem (talvez o type casting esteja errado)
                inLinesize,       // O tamanho da linha vai ser o stride da imagem (naturalmente)
                0, height,        // Tamanho da coluna
                pFrame->data,     // Coloca o valor na struct para a função de codificação
                pFrame->linesize  // Stride da struct
                );

        //Comprimir para h264
        int ret = avcodec_send_frame(avCodecContext, pFrame);
        int amountOfPacketsCreated = 0;

        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "Erro ocorreu: %s (Código: %d)\n", errbuf, ret);
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(avCodecContext, framePacket);

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            currentTimeStamp = timeStampOffset + framePacket->pts * 90000 / 60;
            for(int i = 0;
                    i < clientCount;
                    i++)
            {
                sendRtpPacketh264(&videoStream, &clients[i], currentTimeStamp, framePacket->data, framePacket->size);
            }

            av_packet_unref(framePacket);
        }

        XPutImage(display, win, gc, image, 0, 0, 0, 0, width, height);

        //Destruir imagem
        XDestroyImage(image);
    }

    videoStream.shutdownStream();
    audioStream.shutdownStream();
}
