//
// Created by rokas on 02/09/2024.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>

struct Client {
    //network socket for handling connections
    int socket;
    // variables for specifics of a connection
    int domain;
    int service;
    int protocol;
    int port;
    u_long interface;
    // public member method allows client to make a request of a specified
    char * (*request)(struct Client *client , char *server_ip, char *request);
};

struct Client client_constructor(int domain, int service, int protocol, int port, u_long interface);

#endif //CLIENT_H
