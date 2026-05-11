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
#include "sockets.h"

int main(){
    RtpStream videoStream;
    {
        FunctionReturnStatus status = videoStream.init(0);
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
        FunctionReturnStatus status = audioStream.init(0);
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

    Window win = XCreateSimpleWindow(display, root, 0, 0, width/2, height/2, 1, 0, 0);
    XMapWindow(display, win);
    GC gc = XCreateGC(display, win, 0, NULL);

    while(1){
        //Receber dados
        //Descomprimir os dados
        //Converter os dados de YUV para RGBA/XImage
        //Renderizar os dados
        XImage* image;
        XPutImage(display, win, gc, image, 0, 0, 0, 0, width, height);
    }

    shutdown(videoStream.dataSocket.fd, SHUT_WR);
    shutdown(videoStream.controlSocket.fd, SHUT_WR);
}
