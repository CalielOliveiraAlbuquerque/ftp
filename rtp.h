#pragma once

#include "types.h"
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
