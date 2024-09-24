//
// Created by rokas on 24/09/2024.
//

// client.h
#ifndef CLIENT_H
#define CLIENT_H

void *server_listener(void *arg);
void register_with_server(int sock, const char *username, int listen_port);
void request_peer_list(int sock);

extern int routing_server_sock;
extern int listen_port_global;
extern int in_chat;
extern char username_global[50];

#endif // CLIENT_H
