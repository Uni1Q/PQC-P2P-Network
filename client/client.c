//
// Created by rokas on 24/09/2024.
//

// client.c
#include "client.h"
#include "network.h"
#include "utils.h"
#include "peer_list.h"
#include "chat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int routing_server_sock;
int listen_port_global;
int in_chat = 0;
char username_global[50];

void register_with_server(int sock, const char *username, int listen_port) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "REGISTER %s %d", username, listen_port);
    write(sock, buffer, strlen(buffer));
}

void request_peer_list(int sock) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "GET_PEER_LIST");
    write(sock, buffer, strlen(buffer));
}

void *server_listener(void *arg) {
    char *username = (char *)arg;
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_read = read(routing_server_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            // Connection closed or error
            break;
        }
        buffer[bytes_read] = '\0';

        if (strcmp(buffer, "USERNAME_TAKEN") == 0) {
            // Username is taken, should not happen here
            continue;
        }

        if (strstr(buffer, "New peer connected") != NULL && !in_chat) {
            // Automatically refresh the peer list
            register_with_server(routing_server_sock, username, listen_port_global);
            // Receive updated peer list
            bytes_read = read(routing_server_sock, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                update_peer_list(buffer);
                safe_print("\nPeer list updated. New peers are available.\n");
            }
        }
        // Handle other messages if needed
    }
    pthread_exit(NULL);
}
