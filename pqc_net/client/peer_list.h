//
// Created by rokas on 24/09/2024.
//

// client/peer_list.h

#ifndef PEER_LIST_H
#define PEER_LIST_H

#include <pthread.h>
#include "constants.h"

typedef struct {
    char username[USERNAME_MAX_LENGTH];
    char ip[IP_STR_LEN];
    int port;
} PeerInfo;

#define MAX_PEERS 100

extern PeerInfo peer_list[MAX_PEERS];
extern int peer_count;
extern pthread_mutex_t peer_list_mutex;

// Function Prototypes
void update_peer_list(const char *peer_list_str);
void display_peer_list();

#endif // PEER_LIST_H
