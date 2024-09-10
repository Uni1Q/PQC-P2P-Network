//
// Created by rokas on 02/09/2024.
//

#include "Server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void register_routes_server(struct Server *server, char *(*route_function)(void *arg), char *path);

struct Server server_constructor(int domain, int service, int protocol, u_long interface, int port, int backlog)
{
    struct Server server;
    //basic parameters of the server
    server.domain = domain;
    server.service = service;
    server.protocol = protocol;
    server.interface = interface;
    server.port = port;
    server.backlog = backlog;
    //use the parameters to construct servers' address
    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);
    // create socket for server
    server.socket = socket(domain, service, protocol);
    // Initialize the dictionary.
    server.routes = dictionary_constructor(compare_string_keys);

    server.register_routes = register_routes_server;
    // confirm successful connection
    if (server.socket == 0)
        {
        perror("failed to connect socket\n");
        exit(1);
        }
    // bind socket to the network
    if(bind(server.socket, (struct sockaddr*)&server.address, sizeof(server.address)) < 0) {
        perror("failed to bind socket\n");
        exit(1);
    }
    if(listen(server.socket, server.backlog) < 0)
    {
        perror("failed to listen on socket\n");
        exit(1);
    }
    return server;
}

void register_routes_server(struct Server *server, char *(*route_function)(void *arg), char *path)
{
    struct ServerRoute route;
    route.route_function = route_function;
    server->routes.insert(&server->routes, path, sizeof(char[strlen(path)]), &route, sizeof(route));
}