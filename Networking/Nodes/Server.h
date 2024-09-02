//
// Created by rokas on 02/09/2024.
//

#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

// Datatypes

struct Server {

    int domain;
    int service;
    int protocol;
    u_long interface;
    int port;
    int backlog;
    struct sockaddr_in address;
    int socket;
};

struct Server server_constructor(int domain, int service, int protocol, u_long interface, int port, int backlog, struct sockaddr_in address, int socket);

#endif //SERVER_H
