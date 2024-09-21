/**
 * @file HTTPServer.c
 * @brief Implementation of an HTTP server capable of handling multiple routes and requests using threads.
 * 
 * This file contains the functions required to launch an HTTP server, register routes, 
 * and handle incoming client connections using a thread pool for concurrency.
 * 
 * @date 02/09/2024
 * @version 1.0
 */

#include "HTTPServer.h"
#include "../../Systems/ThreadPool.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* Function prototypes */
void launch(struct HTTPServer *server);
void *handler(void *arg);
void register_routes(struct HTTPServer *server, char *(*route_function)(struct HTTPServer *server, struct HTTPRequest *request), char *uri, int num_methods, ...);

/**
 * @brief Structure to hold a client connection and the associated server for request handling.
 */
struct ClientServer
{
    int client;
    struct HTTPServer *server;
};

/**
 * @brief Structure representing an HTTP route.
 * 
 * The route contains HTTP methods, the URI, and the function associated with handling the route.
 */
struct Route
{
    int methods[9];
    char *uri;
    char *(*route_function)(struct HTTPServer *server, struct HTTPRequest *request);
};

/**
 * @brief Constructs and initializes an HTTP server with default settings.
 * 
 * This function initializes the server, sets up the required socket, and prepares the server 
 * to handle incoming connections.
 * 
 * @return A newly initialized HTTPServer structure.
 */
struct HTTPServer http_server_constructor()
{
    struct HTTPServer server;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); // Clear the struct
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // or a specific IP address
    address.sin_port = htons(80); // Use appropriate port

    // Initialize the server
    server.server = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 80, 255, address);
    server.routes = dictionary_constructor(compare_string_keys);
    server.register_routes = register_routes;
    server.launch = launch;
    return server;
}

/**
 * @brief Registers a route with the HTTP server.
 * 
 * This function adds a route to the server, specifying which HTTP methods are allowed for a particular URI.
 * The route will be associated with a function that handles requests for the URI.
 * 
 * @param server Pointer to the HTTPServer.
 * @param route_function Function to handle requests for the route.
 * @param uri The URI for which the route is registered.
 * @param num_methods The number of HTTP methods allowed for the route.
 * @param ... Variable arguments representing the HTTP methods allowed.
 */
void register_routes(struct HTTPServer *server, char *(*route_function)(struct HTTPServer *server, struct HTTPRequest *request), char *uri, int num_methods, ...)
{
    struct Route route;
    va_list methods;
    va_start(methods, num_methods);

    for (int i = 0; i < num_methods; i++)
    {
        route.methods[i] = va_arg(methods, int);
    }
    va_end(methods);

    // Register the URI.
    char *uri_copy = malloc(strlen(uri) + 1);  /**< Allocate memory for the URI string */
    if (uri_copy == NULL) {
        perror("Failed to allocate memory for URI copy");
        exit(EXIT_FAILURE);
    }
    strcpy(uri_copy, uri);  /**< Copy the URI string into allocated memory */
    route.uri = uri_copy;
    route.route_function = route_function;

    // Save the route in the server's dictionary
    server->routes.insert(&server->routes, uri, strlen(uri) + 1, &route, sizeof(route));
}


/**
 * @brief Launches the HTTP server, accepting incoming connections and handling requests in a thread pool.
 * 
 * This function starts an infinite loop that continuously accepts client connections and passes
 * them to a thread pool for concurrent processing.
 * 
 * @param server Pointer to the HTTPServer.
 */
void launch(struct HTTPServer *server)
{
    // Initialize a thread pool to handle clients
    struct ThreadPool thread_pool = thread_pool_constructor(20);

    // Cast member variables from the server
    struct sockaddr *sock_addr = (struct sockaddr *)&server->server.address;
    socklen_t address_length = (socklen_t)sizeof(server->server.address);

    // Infinite loop to continuously accept new clients
    while (1)
    {
        // Create an instance of the ClientServer struct
        struct ClientServer *client_server = malloc(sizeof(struct ClientServer));
        client_server->client = accept(server->server.socket, sock_addr, &address_length);
        client_server->server = server;

        // Pass the client to the thread pool
        struct ThreadJob job = thread_job_constructor(handler, client_server);
        thread_pool.add_work(&thread_pool, job);
    }
}

/**
 * @brief Handles incoming HTTP requests.
 * 
 * This function is used in a multithreaded environment to process client requests, find the corresponding route, 
 * and return the appropriate response.
 * 
 * @param arg Pointer to the ClientServer structure containing the client connection and server reference.
 * @return NULL after handling the request.
 */
void *handler(void *arg)
{
    // Cast the argument back to a ClientServer struct
    struct ClientServer *client_server = (struct ClientServer *)arg;

    // Read the client's request
    char request_string[30000];
    read(client_server->client, request_string, 30000);

    // Parse the request string into an HTTPRequest structure
    struct HTTPRequest request = http_request_constructor(request_string);

    // Extract the URI from the request
    char *uri = request.request_line.search(&request.request_line, "uri", sizeof("uri"));

    // Find the corresponding route in the server's dictionary
    struct Route *route = client_server->server->routes.search(&client_server->server->routes, uri, sizeof(char[strlen(uri)]));

    // Process the request and send the response to the client
    char *response = route->route_function(client_server->server, &request);
    write(client_server->client, response, strlen(response));
    close(client_server->client);

    // Free memory and destroy the request
    free(client_server);
    http_request_destructor(&request);

    return NULL;
}

/**
 * @brief Combines the contents of multiple files into a single string.
 * 
 * This function reads multiple files and combines their contents into a single string. It can be 
 * used to create a webpage by combining multiple templates.
 * 
 * @param num_templates The number of template files.
 * @param ... Variable arguments representing file paths to be combined.
 * @return A dynamically allocated string containing the combined contents of the files.
 */
char *render_template(int num_templates, ...)
{
    // Create a buffer to store the data in
    char *buffer = malloc(30000);
    int buffer_position = 0;
    char c;
    FILE *file;

    // Iterate over the files given as arguments
    va_list files;
    va_start(files, num_templates);
    for (int i = 0; i < num_templates; i++)
    {
        char *path = va_arg(files, char*);
        file = fopen(path, "r");

        // Read the contents of each file into the buffer
        while ((c = fgetc(file)) != EOF)
        {
            buffer[buffer_position] = c;
            buffer_position += 1;
        }
        fclose(file);  /**< Close the file after reading */
    }
    va_end(files);
    
    return buffer;
}
