#pragma once

#include <string.h>
#include <netinet/in.h>

#include "types.h"
#include "status.h"

struct Ipv6Address{
    uint64 h;
    uint64 l;
};

union IpAddress{
    in6_addr ipv6;
    in_addr ipv4;
};

struct Socket{
    int fd;
    int networkProtocol;
    int transportProtocol;
    int port;
    IpAddress destination;
    char* destinationUri;

    FunctionReturnStatus init(int networkProtocol, int transportProtocol, int port, IpAddress destination){
        this->port = port;
        this->transportProtocol = transportProtocol;
        this->networkProtocol = networkProtocol;
        this->destination = destination;

        if((fd = socket(networkProtocol, SOCK_DGRAM, 0)) < 0) {
            return {1, "Não foi possível criar o descritor de arquivo do socket.\n"};
        }

        switch(networkProtocol){
            case AF_INET6:
            {
                sockaddr_in6 address;
                bzero(&address, sizeof(sockaddr_in6));
                address.sin6_family = AF_INET6;
                address.sin6_port = htons(port);
                memcpy(&address.sin6_addr, &destination.ipv6, sizeof(Ipv6Address));

                if(bind(fd, (sockaddr*)&address, sizeof(address)) < 0) {
                    return FunctionReturnStatus::format(1, "Problema ao associar o socket com a porta %d.\n", port);
                }
            }break;
            case AF_INET:
            {
                sockaddr_in address;
                bzero(&address, sizeof(sockaddr_in));
                address.sin_family = AF_INET6;
                address.sin_port = htons(port);
                address.sin_addr = destination.ipv4;

                if(bind(fd, (sockaddr*)&address, sizeof(address)) < 0) {
                    return FunctionReturnStatus::format(1, "Problema ao associar o socket com a porta %d.\n", port);
                }
            }break;
            default: break;
        }

        return {0, ""};
    }

    void setFlags(){
    }
};

struct RtpStream{
    Socket dataSocket, controlSocket;

    FunctionReturnStatus init(int dataSocketPort){
        IpAddress b;
        b.ipv6 = in6addr_any;
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
};
