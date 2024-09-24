//
// Created by rokas on 24/09/2024.
//

// peer_list.c
#include "peer_list.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

char peer_list[BUFFER_SIZE];
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_peer_list(const char *new_peer_list) {
    pthread_mutex_lock(&peer_list_mutex);
    strncpy(peer_list, new_peer_list, BUFFER_SIZE - 1);
    peer_list[BUFFER_SIZE - 1] = '\0';
    pthread_mutex_unlock(&peer_list_mutex);
}

void display_peer_list() {
    pthread_mutex_lock(&peer_list_mutex);
    char peer_list_copy[BUFFER_SIZE];
    strncpy(peer_list_copy, peer_list, BUFFER_SIZE - 1);
    peer_list_copy[BUFFER_SIZE - 1] = '\0';
    pthread_mutex_unlock(&peer_list_mutex);

    safe_print("List of discoverable peers:\n");
    char *line = strtok(peer_list_copy, "\n");
    int count = 0;
    while (line != NULL) {
        char peer_name[50];
        char peer_address[INET_ADDRSTRLEN];
        int port;
        if (sscanf(line, "%s %s %d", peer_name, peer_address, &port) == 3) {
            safe_print("Username: %s, IP: %s, Port: %d\n", peer_name, peer_address, port);
            count++;
        }
        line = strtok(NULL, "\n");
    }
    if (count == 0) {
        safe_print("No available peers.\n");
    }
}
