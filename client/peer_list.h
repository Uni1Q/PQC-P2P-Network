//
// Created by rokas on 24/09/2024.
//

// peer_list.h
#ifndef PEER_LIST_H
#define PEER_LIST_H

#include <pthread.h>

#define BUFFER_SIZE 1024

extern char peer_list[BUFFER_SIZE];
extern pthread_mutex_t peer_list_mutex;

void update_peer_list(const char *new_peer_list);
void display_peer_list(const char *own_username);

#endif // PEER_LIST_H
