/**
 * @file Client.c
 * @brief Implementation of the Client structure and its methods for handling network requests.
 * 
 * This file contains the implementation of the Client structure's methods, including the
 * ability to send requests to a server and handle responses over a network socket.
 * 
 * @date 02/09/2024
 * @version 1.0
 * @author rokas
 */

#include "Client.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * @brief Sends a request to a specified server and returns the server's response.
 * 
 * This function establishes a connection with the server specified by `server_ip`, 
 * sends the provided request, and reads the server's response. The response is dynamically 
 * allocated, so the caller is responsible for freeing the memory.
 * 
 * @param client Pointer to the Client structure representing the client making the request.
 * @param server_ip The IP address of the server to connect to.
 * @param request The request string to be sent to the server.
 * @return A dynamically allocated string containing the server's response.
 */
char *request(struct Client *client, char *server_ip, char *request, unsigned long size)
{
    // Create an address struct for the server
    struct sockaddr_in server_address;

    // Copy the client's parameters to this address
    server_address.sin_family = client->domain;
    server_address.sin_port = htons(client->port);
    server_address.sin_addr.s_addr = (int)client->interface;

    // Make connection to the server
    inet_pton(client->domain, server_ip, &server_address.sin_addr);
    connect(client->socket, (struct sockaddr*)&server_address, sizeof(server_address));

    // Send the request
    send(client->socket, request, size, 0);

    // Read the response from the server
    char *response = malloc(30000);  /**< Dynamically allocate memory for the response */
    read(client->socket, response, 30000);  /**< Read up to 30000 bytes from the server */
    
    return response;  /**< Return the response string */
}

/**
 * @brief Constructs a new Client object with the specified parameters.
 * 
 * This function creates and initializes a new Client structure with the given network 
 * parameters (domain, service, protocol, port, and interface). The Client is then ready 
 * to make network requests using the `request` method.
 * 
 * @param domain The communication domain (e.g., AF_INET for IPv4).
 * @param service The type of service (e.g., SOCK_STREAM for TCP).
 * @param protocol The specific protocol to be used (e.g., IPPROTO_TCP for TCP).
 * @param port The port number the client will use to connect to the server.
 * @param interface The network interface to be used for communication (usually INADDR_ANY).
 * @return An initialized Client structure.
 */
struct Client client_constructor(int domain, int service, int protocol, int port, long interface)
{
    // Instantiate a Client object
    struct Client client;
    client.domain = domain;
    client.port = port;
    client.interface = interface;

    // Establish socket connection
    client.socket = socket(domain, service, protocol);

    // Set the request method for this client
    client.request = request;

    // Return the constructed Client object
    return client;
}
