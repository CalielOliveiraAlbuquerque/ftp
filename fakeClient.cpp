#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

bool bytecmp(void* bufferArg, int size, size_t value){
    char* buffer = (char*)bufferArg;
    char* valueString = (char*)value;

    for(int i = 0;
            i < size;
            i++)
    {
        if(memcmp(buffer, valueString, value)) return false;
    }

    return true;
}


int main(){
    int clientSocket;
    if((clientSocket = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        printf("Mensagem de erro para quando não é possível criar o socket.\n");
        return 1;
    }

    struct sockaddr_in6 serverAddress;
    {
        in6_addr a;
        bzero(&serverAddress, sizeof(serverAddress));
        serverAddress.sin6_family = AF_INET6;
        serverAddress.sin6_port = htons(8888);
        inet_pton(AF_INET6, "::1", &serverAddress.sin6_addr);  
    }

    if(connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        printf("Problema conectando com o servidor que deveria estar escutando na porta %d.\n", serverAddress.sin6_port);
        return 1;
    }

    const char* message = "olá";
    send(clientSocket, message, strlen(message), 0);
    printf("Mensagem enviada para o servidor: %s.\n", message);
    sleep(1);
    send(clientSocket, message, strlen(message), 0);
}
