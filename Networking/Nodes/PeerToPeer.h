//
// Created by rokas on 02/09/2024.
//

#ifndef PEERTOPEER_H
#define PEERTOPEER_H

#include "Server.h"
#include "../../DataStructures/Lists/LinkedList.h"

struct PeerToPeer
{
    struct Server server;
    struct LinkedList known_hosts;

    int domain;
    int service;
    int protocol;
    int port;
    u_long interface;

    // The user_portal function is activated to launch the server and client applications
    void (*user_portal)(struct PeerToPeer *peer_to_peer);
    void * (*server_function)(void *arg);
    void * (*client_function)(void *arg);
};

struct PeerToPeer peer_to_peer_constructor(int domain, int service, int protocol, int port, u_long interface, void * (*server_function)(void *arg), void * (*client_function)(void *arg));


#endif //PEERTOPEER_H
