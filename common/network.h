//
// Created by rokas on 24/09/2024.
//

// common/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <arpa/inet.h>
#include <sys/socket.h>

int create_socket();
int connect_to_server(const char *ip, int port);
int bind_and_listen(int port);

#endif // NETWORK_H
