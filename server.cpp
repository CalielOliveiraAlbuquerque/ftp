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

//Talvez esteja faltando libavformat
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "types.h"
#include "status.h"
#include "sockets.h"
#include "rtp.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

int socketSinglePoll(int fd, short events, int timeout){
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;

    int result = poll(&pfd, 1, timeout);

    return result > 0? pfd.revents : result;
}

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

        for(int i = 0;
                i < 8;
                i++)
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

const int videoStreamDataPort = 5050;
const int audioStreamDataPort = 5052;

int main(){
    RtpStream videoStream;
    {
        FunctionReturnStatus status = videoStream.init(videoStreamDataPort);
        if(status.error > 0){
            printf("Erro ao criar stream de vídeo por -> %s\n", status.message);
            return 0;
        }
    }
    printf("Stream de vídeo criada.\n");
    printf("Porta do socket de dados: %d.\n", videoStream.dataSocket.fd);
    printf("Porta do socket de controle: %d.\n", videoStream.controlSocket.fd);
    printf("\n");

    //Parte secundária
    RtpStream audioStream;
    {
        FunctionReturnStatus status = audioStream.init(audioStreamDataPort);
        if(status.error > 0){
            printf("Erro ao criar stream de áudio por -> %s.\n", status.message);
            return 0;
        }
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
    pFrame->format = AV_PIX_FMT_YUV420P;
    pFrame->width  = width;
    pFrame->height = height;
    const AVCodec* h264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!h264Codec){
        printf("Não há o codec para h264.\n");
        return 1;
    }
    AVCodecContext* avCodecContext = avcodec_alloc_context3(h264Codec);
    avCodecContext->bit_rate = 400000;         // Adjust based on desired quality
    avCodecContext->width = width;             // From your XWindowAttributes
    avCodecContext->height = height;
    avCodecContext->time_base = (AVRational){1, 60}; // 60 FPS
    avCodecContext->framerate = (AVRational){60, 1};
    avCodecContext->gop_size = 10;             // Intra-frames every 10 frames
    avCodecContext->max_b_frames = 1;
    avCodecContext->pix_fmt = AV_PIX_FMT_YUV420P; // Must match your sws_scale output

    av_frame_get_buffer(pFrame, 0);

    while(1){
        //Pegar imagem da tela
        XImage *image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
        
#if 0
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
        // 1. Send the raw frame to the encoder
        int ret = avcodec_send_frame(avCodecContext, pFrame);

        AVPacket* packet;
        while (ret >= 0) {
            // 2. Receive the compressed packet back
            ret = avcodec_receive_packet(avCodecContext, packet);

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

            // 3. Packet is now encoded! Save it to a file or stream it.
            // (This is your .mp4 data)

            av_packet_unref(packet);
        }
#endif
        //Transformar em pacotes
        //Destruir imagem
        XDestroyImage(image);
    }
    XCloseDisplay(display);

    shutdown(videoStream.dataSocket.fd, SHUT_WR);
    shutdown(videoStream.controlSocket.fd, SHUT_WR);
}
