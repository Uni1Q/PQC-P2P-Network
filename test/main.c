#include "../netlib.h"

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/unistd.h>

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

void client_function(char *request)
{
    struct Client client = client_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 1248);
    client.request(&client, "127.0.0.1", request);
}

int main(void) {

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_function, NULL);

    while(1) {
        char request[512];
        memset(request, 0, 512);
        fgets(request, 512, stdin);
        client_function(request);
    }
}
