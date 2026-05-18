#pragma once

#include <string.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>

#include "types.h"
#include "status.h"

union IpAddress{
    in6_addr ipv6;
    in_addr ipv4;
};

struct Socket{
    int fd;
    int networkProtocol;
    int transportProtocol;
    int port;
    IpAddress ipAddress;

    FunctionReturnStatus init(int networkProtocol, int transportProtocol, int port, IpAddress ipAddress){
        this->port = port;
        this->transportProtocol = transportProtocol;
        this->networkProtocol = networkProtocol;
        this->ipAddress = ipAddress;

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
                memcpy(&address.sin6_addr, &ipAddress.ipv6, sizeof(in6_addr));

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
                address.sin_addr = ipAddress.ipv4;

                if(bind(fd, (sockaddr*)&address, sizeof(address)) < 0) {
                    return FunctionReturnStatus::format(1, "Problema ao associar o socket com a porta %d.\n", port);
                }
            }break;
            default: break;
        }

        return {0, ""};
    }

    void printAddress(){
        switch(networkProtocol){
            case AF_INET6:
            {
                char ipString[INET6_ADDRSTRLEN];

                if(memcmp(&ipAddress.ipv6, &in6addr_any, sizeof(struct in6_addr)) == 0){
                    //por enquanto vai ser assim
                    if (inet_ntop(AF_INET6, &(ipAddress.ipv6), ipString, sizeof(ipString)) != NULL) {
                        printf("%s", ipString);
                    } else{
                        printf("----:----:----:----:----:----:----:----");
                    }
                }else{
                    if (inet_ntop(AF_INET6, &(ipAddress.ipv6), ipString, sizeof(ipString)) != NULL) {
                        printf("%s", ipString);
                    } else{
                        printf("----:----:----:----:----:----:----:----");
                    }
                }
            }break;
            case AF_INET:
            {
            }break;
            default: break;
        }
    }

    void setFlags(){
    }
};


inline
int socketSinglePoll(int fd, short events, int timeout){
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;

    int result = poll(&pfd, 1, timeout);

    return result > 0? pfd.revents : result;
}
