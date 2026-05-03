#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "types.h"
#include <fcntl.h>
#include <poll.h>
#include <assert.h>

#define MAX_MESSAGE_SIZE KB(1024)

struct Status{
    const char* message;
    int value;
};

struct String{
    char* content;
    int size;
    int space;

    char& operator[](size_t index){
        return content[index];
    }

    const char& operator[](size_t index) const{
        return content[index];
    }
};

struct Report{
    uint32 ssrc;
    struct{
        uint8 fractionLost : 8;
        uint32 packetsLostCount : 24;
    };
    //extended highest sequence number received 
    uint32 ehsnr;
    //interarrival jitter 
    uint32 ij;
    uint32 lsr;
    uint32 dlsr;
}report;

struct RtpPacket{
    struct{
        uint8 version : 2;
        bool padding : 1;
        bool extension : 1;
        uint8 csrcCount : 4;
    };
    struct{
        bool marker : 1;
        uint8 payloadTime : 7;
    };
    uint16 sequenceNumber;
    uint32 timestamp;
    //SSRC é o acrônimo de Sychronization SouRCe
    uint32 ssrc;
    //CSRC é o acrônimo de Contributing SouRCe
    //lista de csrc que pode ir de 0 a 15 items
    uint32 csrc[];
};

//data transfer protocol
//Isso pode ser acoplado ao header a depender da aplicação
struct RtpHeaderExtension{
    uint16 applicationDependent;
    uint16 length;
    //essa parte pode ser qualquer informação com tamanho length
    char extension[];
};

struct SenderReportRtcpPacket{
    struct{
        uint8 version : 2;
        bool padding : 1;
        uint8 rc : 5;
    };
    uint8 packetType;
    uint16 length;
    uint32 ssrc;
    struct{
        uint32 h;
        uint32 l;
    }timestamp;

    uint32 rtpTimestamp;
    uint32 packetCount;
    uint32 octetCount;

    //Número de reports vai de 0 a 31
    Report reports[];

    //Depois poderia ter profileEspecific extension (6.4.3 fala sobre isso)
};

struct ReceiverReportRtcpPacket{
    struct{
        uint8 version : 2;
        bool padding : 1;
        uint8 rc : 5;
    };
    uint8 packetType;
    uint16 length;
    uint32 sscr;

    Report reports[];
    //Depois poderia ter profileEspecific extension (6.4.3 fala sobre isso)
};

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

int main(){
    int dataSocket;
    int controlSocket;
    if((dataSocket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0 || (controlSocket = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        printf("Não foi possível criar os sockets necessários para o servidor.\n");
        return 1;
    }
    {
        int returnValue = 1;
        setsockopt(dataSocket, SOL_SOCKET, SO_REUSEADDR, &returnValue, sizeof(returnValue));
        setsockopt(controlSocket, SOL_SOCKET, SO_REUSEADDR, &returnValue, sizeof(returnValue));
    }

    struct sockaddr_in6 serverAddress;
    {
        bzero(&serverAddress, sizeof(serverAddress));
        serverAddress.sin6_family = AF_INET6;
        serverAddress.sin6_port = htons(8080);
        serverAddress.sin6_addr = in6addr_any;
    }

    if(bind(dataSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0 ||
            bind(controlSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("Problema ao associar o socket com o endereço de porta %d.\n", ntohs(serverAddress.sin6_port));
        return 1;
    }

    {
        int currentFlags  = fcntl(dataSocket, F_GETFL, 0);
        if(currentFlags == -1 || fcntl(dataSocket, F_SETFL, currentFlags | O_NONBLOCK) < 0){
            printf("Não foi possível mudar o socket para não bloqueante.\n");
            return 1;
        }
    }
    {
        int currentFlags  = fcntl(controlSocket, F_GETFL, 0);
        if(currentFlags == -1 || fcntl(controlSocket, F_SETFL, currentFlags | O_NONBLOCK) < 0){
            printf("Não foi possível mudar o socket para não bloqueante.\n");
            return 1;
        }
    }

    while(1){
    }

    shutdown(dataSocket, SHUT_WR);
}
