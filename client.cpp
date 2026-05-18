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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "types.h"
#include "status.h"
#include "rtp.h"
#include "program.h"
#include "sockets.h"
#include "server.h"

#include "program.cpp"

int main(int argc, char** argv){
    IpAddress serverAddress;
    if(argc == 1){
        serverAddress.ipv6 = in6addr_loopback;
    }else if(argc == 2){
        inet_pton(AF_INET6, argv[1], &serverAddress.ipv6);
    }else{
        return 0;
    }

    const int clientScrc = random();
    RtpStream serverVideoStream;
    {
        FunctionReturnStatus status = serverVideoStream.init(&serverAddress, serverVideoStreamDataPort);
        if(status.error > 0){
            printf("Erro ao criar stream de vídeo por -> %s\n", status.message);
            return 0;
        }
    }
    RtpStream serverAudioStream;
    {
        FunctionReturnStatus status = serverAudioStream.init(&serverAddress, serverAudioStreamDataPort);
        if(status.error > 0){
            printf("Erro ao criar stream de vídeo por -> %s\n", status.message);
            return 0;
        }
    }

    RtpStream clientVideoStream;
    {
        FunctionReturnStatus status = clientVideoStream.init(0);
        if(status.error > 0){
            printf("Erro ao criar stream de vídeo por -> %s\n", status.message);
            return 0;
        }
    }
    clientVideoStream.ssrc = clientScrc;
    printf("Stream de vídeo criada.\n");
    printf("Porta do socket de dados: %d.\n", clientVideoStream.dataSocket.fd);
    printf("Porta do socket de controle: %d.\n", clientVideoStream.controlSocket.fd);
    printf("\n");

    //Parte secundária
    RtpStream clientAudioStream;
    {
        FunctionReturnStatus status = clientAudioStream.init(0);
        if(status.error > 0){
            printf("Erro ao criar stream de áudio por -> %s.\n", status.message);
            return 0;
        }
    }
    clientAudioStream.ssrc = clientScrc + 1;
    printf("Stream de áudio criada.\n");
    printf("Porta do socket de dados: %d.\n", clientAudioStream.dataSocket.fd);
    printf("Porta do socket de controle: %d.\n", clientAudioStream.controlSocket.fd);
    printf("\n");

    Display *display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);
    XWindowAttributes gwa;

    XGetWindowAttributes(display, root, &gwa);
    int width = gwa.width;
    int height = gwa.height;

    Window win = XCreateSimpleWindow(display, root, 0, 0, width/2, height/2, 1, 0, 0);
    XMapWindow(display, win);
    GC gc = XCreateGC(display, win, 0, NULL);

    uint8 timestampRolloverCounter = 0;

    while(1)
    {
        Message connectMeMessage;
        connectMeMessage.messageType = connectMe;
        connectMeMessage.stream = clientVideoStream;
        sendRtpMessage(&clientVideoStream, &serverVideoStream, &connectMeMessage);
        socketSinglePoll(clientVideoStream.dataSocket.fd, POLLIN, 300);

        while(1){
            socketSinglePoll(clientVideoStream.dataSocket.fd, POLLIN, 300);
            //Receber dados

            //Descomprimir os dados

            //Converter os dados de YUV para RGBA/XImage

            XImage* image;
            //Renderizar os dados
            XPutImage(display, win, gc, image, 0, 0, 0, 0, width, height);
        }
    }

    shutdown(clientVideoStream.dataSocket.fd, SHUT_WR);
    shutdown(clientVideoStream.controlSocket.fd, SHUT_WR);
}
