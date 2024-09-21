#include "../netlib.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

#include "../Networking/Nodes/PeerToPeer.h"
#include "../Networking/Protocols/BlockHeaders.h"

void *server_function(void *arg)
{
    const unsigned long MAX_BLOCK_SIZE = 12345678;

    printf("Server running.\n");
    struct PeerToPeer *p2p = (struct PeerToPeer *)arg;
    struct sockaddr *address = (struct sockaddr *)&p2p->server.address;
    socklen_t address_length = (socklen_t)sizeof(p2p->server.address);
    while (1)
    {
        int client = accept(p2p->server.socket, address, &address_length);
        if (client < 0) {
            perror("accept");
            continue;
        }

        void *request = calloc(1, MAX_BLOCK_SIZE);
        if (request == NULL) {
            perror("calloc");
            close(client);
            continue;
        }

        ssize_t bytes_read = read(client, request, MAX_BLOCK_SIZE);
        if (bytes_read < 0) {
            perror("read");
            free(request);
            close(client);
            continue;
        }

        struct BlockHeaders *headers = (struct BlockHeaders *)request;
        struct Block *block = calloc(1, headers->size);
        if (block == NULL) {
            perror("calloc");
            free(request);
            close(client);
            continue;
        }
        memcpy(block, request, headers->size);

        char *client_address = inet_ntoa(p2p->server.address.sin_addr);

        // Check the request before freeing it
        if (strcmp((char *)request, "/known_hosts\n") == 0)
        {
            struct ServerRoute *route = p2p->server.routes.search(&p2p->server.routes, (char *)request, strlen((char *)request));
            if (route == NULL) {
                perror("search");
                free(block);
                free(request);
                close(client);
                continue;
            }
            char *response = route->route_function(arg);
            if (response == NULL) {
                perror("route_function");
                free(block);
                free(request);
                close(client);
                continue;
            }
            ssize_t bytes_written = write(client, response, strlen(response));
            if (bytes_written < 0) {
                perror("write");
            }
            close(client);
        }
        else
        {
            printf("\t\t\t%s says: \n\n\t\t\tPrevious: %s\n\t\t\tNonce: %lu\n\t\t\tBy: %s\n\t\t\tTimestamp: %s\n\t\t\tSize: %lu\n\n\t\t\t%d\n\n",
       client_address,
       (char *)block->headers.previous,  // Casting to char* if it's a null-terminated string
       block->headers.nonce,
       (char *)block->headers.by,  // Casting to char* if it's a null-terminated string
       block->headers.timestamp,
       block->headers.size,
       block->data);  // Assuming this is an integer

        }

        short found = 0;
        for (int i = 0; i < p2p->known_hosts.length && !found; i++)
        {
            if (strcmp(client_address, p2p->known_hosts.retrieve(&p2p->known_hosts, i)) == 0)
            {
                found = 1;
            }
        }
        if (!found)
        {
            p2p->known_hosts.insert(&p2p->known_hosts, p2p->known_hosts.length, client_address, sizeof(client_address));
        }
        free(block);
        free(request);  // Move this line to the end to avoid use-after-free
    }
    return NULL;
}

void *client_function(void *arg)
{
    struct PeerToPeer *p2p = arg;
    while (1)
    {
        clock_t start = clock();
        struct Client client = client_constructor(p2p->domain, p2p->service, p2p->protocol, p2p->port, p2p->interface);
        char request[255];
        memset(request, 0, sizeof(request));
        if (fgets(request, sizeof(request), stdin) == NULL) {
            perror("fgets");
            continue;
        }
        for (int i = 0; i < p2p->known_hosts.length; i++)
        {
            struct BlockHeaders headers;
            for(int j = 0; j < 63; j++)
            {
                headers.previous[j] = j + 60;
                headers.by[j] = j + 60;
            }
            headers.nonce = 123;
            strcpy(headers.timestamp, "2024-09-02 12:00:00");
            headers.size = sizeof(struct BlockHeaders) + strlen(request);
            void *block = calloc(headers.size, 1);
            if (block == NULL) {
                perror("calloc");
                continue;
            }
            memcpy(block, &headers, sizeof(struct BlockHeaders));
            memcpy((char *)block + sizeof(struct BlockHeaders), request, strlen(request));

            char *response = client.request(&client, p2p->known_hosts.retrieve(&p2p->known_hosts, i), block);
            if (response != NULL) {
                printf("%s\n", response);
            }
            free(block);
        }
        clock_t end = clock();
        if ((end - start) > 500)
        {
            char *response = client.request(&client, p2p->known_hosts.retrieve(&p2p->known_hosts, 0), "/known_hosts\n");
            if (response != NULL) {
                printf("%s\n", response);
            }
        }
    }
}

int int_compare(void *a, void *b)
{
    int *x = a;
    int *y = b;
    if (*x > *y)
    {
        return 1;
    }
    else if (*x < *y)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int main()
{
    printf("%lu, %lu\n", sizeof(struct BlockHeaders), sizeof(struct Block));
    struct PeerToPeer p2p = peer_to_peer_constructor(AF_INET, SOCK_STREAM, 0, 1248, INADDR_ANY, server_function, client_function);
    p2p.user_portal(&p2p);

    return 0;
}
