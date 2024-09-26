//
// Created by rokas on 24/09/2024.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>

extern int server_sock;
extern char username_global[50];
extern int peer_port;
extern volatile int in_chat;

void *server_listener(void *arg);
void *peer_listener(void *arg);
void *handle_incoming_peer(void *arg);

void request_peer_list(int server_sock);

#endif // CLIENT_H