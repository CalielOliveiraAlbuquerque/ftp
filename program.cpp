#include <unistd.h>

#include "program.h"
#include "server.h"

void sendRtpMessage(const RtpStream* source, const RtpStream* destination, Message* message){
    uint8 packetBuffer[rtpMaximumPacketSize];

    RtpHeader header;
    header.version = 2;
    header.csrcCount = 0;
    header.extension = 0;
    header.padding = 0;
    header.payloadType = customMessageType;
    header.marker = 0;
    header.timestamp = htonl(0);
    header.sequenceNumber = htons(0);
    header.ssrc = htonl(source->ssrc);

    memcpy(packetBuffer, &header, sizeof(RtpHeader));
    memcpy(packetBuffer + sizeof(RtpHeader), message, sizeof(Message));

    write(destination->dataSocket.fd, packetBuffer, sizeof(RtpHeader) + sizeof(Message));
}

