//
// Created by rokas on 24/09/2024.
//

#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include "constants.h"

typedef struct {
	char username[USERNAME_MAX_LENGTH];
	char ip[INET_ADDRSTRLEN];
	int port;
} Peer;

int create_socket();
int connect_to_server(const char *ip, int port);
int bind_and_listen(int port);
int connect_to_peer(const char *ip, int port);

#endif // NETWORK_H
