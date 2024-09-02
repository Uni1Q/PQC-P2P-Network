#include "../netlib.h"

#include <stdio.h>
#include <pthread.h>
#include <string.h>

void * server_function(void *arg)
{
    struct Server server = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 1248, 20);
    struct sockaddr *address = (struct sockaddr *) &server.address;
    socklen_t address_length = (socklen_t) sizeof(server.address);
    while (1) {
        int client = accept(server.socket, address, &address_length);
        char request[512];
        memset(request, 0, 512);
        write(client , request, 512);
        printf("%s\n", request);
        close(client);
    }
    return NULL;
}

void * client_function(char *request)
{
    struct Client client = client_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 1248);
}

int main(void) {

    pthread_t server_thread;
    pthread_create(server_thread, NULL, server_function, NULL);

    return 0;
}
