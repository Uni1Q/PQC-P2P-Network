//
// Created by rokas on 24/09/2024.
//

// client/peer_list.c

#include "peer_list.h"
#include "utils.h"
#include <stdlib.h>  // For malloc(), free()
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

PeerInfo peer_list[MAX_PEERS];
int peer_count = 0;
pthread_mutex_t peer_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_peer_list(const char *peer_list_str) {
    pthread_mutex_lock(&peer_list_mutex);
    peer_count = 0;

    char *str = strdup(peer_list_str);
    if (str == NULL) {
        perror("strdup");
        pthread_mutex_unlock(&peer_list_mutex);
        return;
    }
    char *line = strtok(str, "\n");

    while (line != NULL && peer_count < MAX_PEERS) {
        char username[USERNAME_MAX_LENGTH];
        char ip[IP_STR_LEN];
        int port;

        if (sscanf(line, "%s %s %d", username, ip, &port) == 3) {
            strncpy(peer_list[peer_count].username, username, USERNAME_MAX_LENGTH - 1);
            peer_list[peer_count].username[USERNAME_MAX_LENGTH - 1] = '\0';

            strncpy(peer_list[peer_count].ip, ip, IP_STR_LEN - 1);
            peer_list[peer_count].ip[IP_STR_LEN - 1] = '\0';

            peer_list[peer_count].port = port;
            peer_count++;
        }
        line = strtok(NULL, "\n");
    }

    free(str);
    pthread_mutex_unlock(&peer_list_mutex);
}

void display_peer_list() {
    pthread_mutex_lock(&peer_list_mutex);
    safe_print("List of discoverable peers:\n");
    if (peer_count == 0) {
        safe_print("No available peers.\n");
    } else {
        for (int i = 0; i < peer_count; i++) {
            safe_print("Username: %s, IP: %s, Port: %d\n",
                peer_list[i].username,
                peer_list[i].ip,
                peer_list[i].port);
        }
    }
    pthread_mutex_unlock(&peer_list_mutex);
}
