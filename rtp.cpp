#include <unistd.h>
#include <random>

#include "sockets.h"
#include "rtp.h"
#include "program.h"

void sendRtpPacketh264(const RtpStream* source, const RtpStream* destination, const uint32 timestamp, const void* data, const size_t dataSize){
    const char* dataBuffer = (const char*)data;

    const char* nextNaluStart = dataBuffer;
    bool lastNalu = false;
    size_t i = 0;
    const uint16 sequenceNumberOffset = random();
    uint16 currentSequenceNumber = sequenceNumberOffset;
    while(1)
    {
        size_t numberOfBytesInNalu = 0;
        const char* oldNaluStart = nextNaluStart;
        const char* startCodePointer24 = (const char*)memmem(nextNaluStart, dataBuffer + dataSize - nextNaluStart, annexBStartCode24, 3);
        if(!startCodePointer24){
            numberOfBytesInNalu = dataBuffer + dataSize - oldNaluStart;
            nextNaluStart = NULL;
            lastNalu = true;
        }else{
            nextNaluStart = startCodePointer24 + 3;
            numberOfBytesInNalu = nextNaluStart - oldNaluStart;
        }

        if(numberOfBytesInNalu - 3 > rtpMaximumPacketSize - sizeof(RtpHeader)){
            printf("Tamnho do pacote tem que ser dividido.\n");
            getchar();
        }

        uint8 packetBuffer[rtpMaximumPacketSize];

        RtpHeader header;
        header.version = 2;
        header.csrcCount = 0;
        header.extension = 0;
        header.padding = 0;
        header.payloadType = videoStreamingType;
        header.marker = lastNalu;

        header.timestamp = htonl(timestamp);
        header.sequenceNumber = htons(currentSequenceNumber);
        header.ssrc = htonl(destination->ssrc);

        memcpy(packetBuffer, &header,  sizeof(RtpHeader));

        memcpy(packetBuffer + sizeof(RtpHeader), oldNaluStart + 3, numberOfBytesInNalu - 3);

        write(destination->dataSocket.fd, packetBuffer, udpHeaderSize + sizeof(RtpHeader) + numberOfBytesInNalu - 3);

        currentSequenceNumber++;

        if(lastNalu){
            break;
        }

        i++;
    }
}
