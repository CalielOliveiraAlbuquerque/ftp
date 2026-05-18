#pragma once

#include "rtp.h"

enum MessageType{
    connectMe,
};

struct Message{
    MessageType messageType;
    union{
        RtpStream stream;
    };
};

inline Message connectMeMessage = {connectMe};

const int videoStreamingType = 96;
const int customMessageType = 97;

void sendRtpMessage(const RtpStream* source, const RtpStream* destination, Message* message);

