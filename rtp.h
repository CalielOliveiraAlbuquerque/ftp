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

struct BitRange{
    uint8 start;
    uint8 length;
};

static const BitRange rtpVersion = {6, 2};
static const BitRange rtpPadding = {5, 1};
static const BitRange rtpExtension = {4, 1};
static const BitRange rtpCsrcCount = {0, 4};

static const BitRange rtpMarker = {6, 1};
static const BitRange rtpPayLoadTime = {0, 7};

static const BitRange rtpRc = {3, 5};

inline
void setByte(uint8* destination, uint8 value, const BitRange* range){
    uint8 bitsSetMask = (1 << range->length) - 1; //1000 - 1 = 111
    *destination &= ~(bitsSetMask << range->start); //zera a parte a esquerda não utilizada
    *destination |= (value & bitsSetMask) << range->start; //mask & bitsSetMask faz com que o valor de mask só vá até onde range 'length'
}

inline
uint8 getEquivalentByte(uint8 destination, const BitRange* range){
    destination <<= 8 - (range->start + range->length);
    return destination >> (8 - range->length);
}

struct RtpPacket{
    uint8 firstByte;
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
    uint8 firstByte;
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
    uint8 firstByte;
    uint8 packetType;
    uint16 length;
    uint32 sscr;

    Report reports[];
    //Depois poderia ter profileEspecific extension (6.4.3 fala sobre isso)
};

void setByte(uint8* destination, uint8 mask, const BitRange* range);
