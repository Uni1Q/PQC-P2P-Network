/**
 * @file client.h
 * @brief Header file defining the Client structure and related methods for handling network connections.
 * 
 * This header provides an interface for creating and managing a client connection
 * across different platforms (Windows and Unix-based systems).
 * 
 * @date 02/09/2024
 * @author rokas
 */

#ifndef CLIENT_H
#define CLIENT_H

// Platform-specific includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    /**
     * @typedef socket_t
     * @brief Defines the type for network sockets on Windows.
     */
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    /**
     * @typedef socket_t
     * @brief Defines the type for network sockets on Unix-based systems.
     */
    typedef int socket_t;
#endif

/**
 * @struct Client
 * @brief Represents a network client that manages socket-based connections.
 * 
 * This structure holds the necessary details for creating and maintaining
 * a network connection, and provides a method for making requests to a server.
 */
struct Client {
    socket_t socket; /**< The socket associated with this client for network communication. */
    int domain;      /**< The domain of the connection, such as AF_INET for IPv4. */
    int service;     /**< The service type, typically SOCK_STREAM for TCP. */
    int protocol;    /**< The protocol to be used, like IPPROTO_TCP for TCP connections. */
    int port;        /**< The port number on which the client will connect. */
    long interface;  /**< The IP address or interface on which to bind, e.g., INADDR_ANY. */

    /**
     * @brief Makes a request to a specified server.
     * 
     * This method sends a request to the server at the specified IP address
     * and returns the response received from the server.
     * 
     * @param client The Client structure pointer to use for the connection.
     * @param server_ip The IP address of the server to which the request will be made.
     * @param request The request message to send to the server.
     * @return The response received from the server, or NULL if an error occurred.
     */
    char *(*request)(struct Client *client, char *server_ip, char *request, unsigned long size);
};

/**
 * @brief Constructs a new Client object.
 * 
 * This function initializes a new Client structure with the provided parameters.
 * The client will use the specified domain, service, protocol, port, and interface.
 * 
 * @param domain The domain of the connection, such as AF_INET for IPv4.
 * @param service The service type, typically SOCK_STREAM for TCP.
 * @param protocol The protocol to be used, like IPPROTO_TCP for TCP connections.
 * @param port The port number on which the client will connect.
 * @param interface The IP address or interface on which to bind, e.g., INADDR_ANY.
 * @return A Client structure initialized with the provided values.
 */
struct Client client_constructor(int domain, int service, int protocol, int port, long interface);

#endif //CLIENT_H
