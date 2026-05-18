#pragma once

#include "types.h"
#include "sockets.h"
#include <fcntl.h>

#define MAX_MESSAGE_SIZE KB(1024)

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
};

struct RtpHeader{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8 csrcCount:4;   
    uint8 extension:1;   
    uint8 padding:1;     
    uint8 version:2;     
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8 version:2;     
    uint8 padding:1;     
    uint8 extension:1;   
    uint8 csrcCount:4;   
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8 payloadType:7; 
    uint8 marker:1;      
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8 marker:1;      
    uint8 payloadType:7; 
#endif

    uint8 secondByte;
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
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8 reportCount:5; 
    uint8 padding:1;     
    uint8 version:2;     
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8 version:2;     
    uint8 padding:1;     
    uint8 reportCount:5; 
#endif
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
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8 reportCount:5; 
    uint8 padding:1;     
    uint8 version:2;     
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8 version:2;     
    uint8 padding:1;     
    uint8 reportCount:5; 
#endif
    uint8 packetType;
    uint16 length;
    uint32 sscr;

    Report reports[];
    //Depois poderia ter profileEspecific extension (6.4.3 fala sobre isso)
};

struct RtpStream{
    Socket dataSocket, controlSocket;
    int ssrc;

    FunctionReturnStatus init(IpAddress* ipAddress, int dataSocketPort){
        IpAddress b;
        b.ipv6 = ipAddress->ipv6;
        FunctionReturnStatus result = dataSocket.init(AF_INET6, SOCK_DGRAM, dataSocketPort, b);
        if(result.error){
            return FunctionReturnStatus::format(1, "Erro ao criar o socket de dados por -> %s", result.message);
        }
        result = controlSocket.init(AF_INET6, SOCK_DGRAM, dataSocketPort + 1, b);
        if(result.error){
            return FunctionReturnStatus::format(1, "Erro ao criar o socket de controle por -> %s", result.message);
        }

        return {0, ""};
    }

    FunctionReturnStatus init(int dataSocketPort){
        return init((IpAddress*)&in6addr_loopback, dataSocketPort);
    }

    void shutdownStream(){
        shutdown(dataSocket.fd, SHUT_RDWR);
        shutdown(controlSocket.fd, SHUT_RDWR);
    }
};

const int mtuSize = 1500;
const int udpHeaderSize = 8;
const int rtpMaximumPacketSize = mtuSize - udpHeaderSize;
const char annexBStartCode24[3] = {0x00, 0x00, 0x01};
