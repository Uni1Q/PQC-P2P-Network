#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void register_routes_server(struct Server *server, char *(*route_function)(void *arg), char *path);

struct Server server_constructor(int domain, int service, int protocol, long interface, int port, int backlog, struct sockaddr_in address)
{
    struct Server server;  // Properly initialize the struct

    // Basic parameters of the server
    server.domain = domain;
    server.service = service;
    server.protocol = protocol;
    server.interface = interface;  // Use the renamed field
    server.port = port;
    server.backlog = backlog;
    server.address = address;

    // Construct the server's address
    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);

    // Create socket for server
    server.socket = socket(domain, service, protocol);

    // Initialize the dictionary
    server.routes = dictionary_constructor(compare_string_keys);
    server.register_routes = register_routes_server;

    // Confirm successful connection
    if (server.socket == INVALID_SOCKET)
    {
        perror("Failed to create socket\n");
        exit(1);
    }

    // Bind socket to the network
    if (bind(server.socket, (struct sockaddr*)&server.address, sizeof(server.address)) < 0)
    {
        perror("Failed to bind socket\n");
        exit(1);
    }

    // Listen on the socket
    if (listen(server.socket, server.backlog) < 0)
    {
        perror("Failed to listen on socket\n");
        exit(1);
    }

    return server;
}

void register_routes_server(struct Server *server, char *(*route_function)(void *arg), char *path)
{
    struct ServerRoute route;
    route.route_function = route_function;

    server->routes.insert(&server->routes, path, strlen(path) + 1, &route, sizeof(route));
}
