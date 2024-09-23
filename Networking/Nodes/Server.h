/**
 * @file server.h
 * @brief Header file defining the Server structure and related methods for handling server-side network connections.
 * 
 * This header provides an interface for creating and managing a server that can handle
 * incoming connections, register routes, and respond to client requests.
 * 
 * @date 02/09/2024
 * @author rokas
 */

#ifndef SERVER_H
#define SERVER_H

#define INVALID_SOCKET -1
#include "../../DataStructures/Dictionary/Dictionary.h"
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * @struct Server
 * @brief Represents a network server that handles incoming client connections and registered routes.
 * 
 * This structure holds information about the server's configuration, including domain, service, protocol,
 * port, and routes. It also provides functions to register routes and respond to client requests.
 */
struct Server
{
    int domain;              /**< The domain of the connection, such as AF_INET for IPv4. */
    int service;             /**< The service type, typically SOCK_STREAM for TCP. */
    int protocol;            /**< The protocol to be used, like IPPROTO_TCP for TCP connections. */
    long interface;          /**< The IP address or interface on which to bind, e.g., INADDR_ANY. */
    int port;                /**< The port number on which the server will listen. */
    int backlog;             /**< The maximum number of pending connections in the server’s listen queue. */
    struct sockaddr_in address; /**< The server’s socket address, which holds IP and port details. */
    int socket;              /**< The server’s socket file descriptor, used for accepting connections. */

    struct Dictionary routes; /**< A dictionary structure for mapping routes to specific functions. */

    /**
     * @brief Registers a route with the server.
     * 
     * This function associates a specific route (path) with a function that will handle
     * requests to that route. The function will be called when a client sends a request
     * to the registered path.
     * 
     * @param server The Server structure pointer managing the server.
     * @param route_function The function pointer that will handle requests to the route.
     * @param path The URL path to be associated with the route function.
     */
    void (*register_routes)(struct Server *server, char *(*route_function)(void *arg), char *path);
};

/**
 * @struct ServerRoute
 * @brief Represents a route in the server, linking a specific URL path to a function.
 * 
 * This structure contains a function pointer that will be called when a client
 * requests the specific route.
 */
struct ServerRoute
{
    /**
     * @brief The URL path or command for this route.
     * 
     * This is the path or identifier associated with the route.
     */
    char *route_name;

    /**
     * @brief The function to be executed when the route is accessed.
     * 
     * This function will handle requests to the associated route.
     * 
     * @param arg The argument passed to the route function.
     * @return A pointer to the result of the route function.
     */
    char *(*route_function)(void *arg);
};


/**
 * @brief Constructs a new Server object.
 * 
 * This function initializes a Server structure with the provided parameters.
 * The server will be set up with the specified domain, service, protocol, port, and address,
 * and it will be prepared to accept incoming connections.
 * 
 * @param domain The domain of the connection, such as AF_INET for IPv4.
 * @param service The service type, typically SOCK_STREAM for TCP.
 * @param protocol The protocol to be used, like IPPROTO_TCP for TCP connections.
 * @param interface The IP address or interface on which to bind, e.g., INADDR_ANY.
 * @param port The port number on which the server will listen.
 * @param backlog The maximum number of pending connections in the server’s listen queue.
 * @param address The server's socket address.
 * @return A Server structure initialized with the provided values.
 */
struct Server server_constructor(int domain, int service, int protocol, long interface, int port, int backlog, struct sockaddr_in address);

#endif //SERVER_H
