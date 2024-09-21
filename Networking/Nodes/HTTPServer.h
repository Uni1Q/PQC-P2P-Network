/**
 * @file http_server.h
 * @brief Header file defining the HTTPServer structure and methods for handling HTTP server functionality.
 * 
 * This header provides an interface for setting up an HTTP server, registering routes,
 * handling HTTP requests, and serving responses. It includes support for multiple HTTP methods
 * and templating for web page generation.
 * 
 * @date 02/09/2024
 * @author rokas
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "Server.h"
#include "../Protocols/HTTPRequest.h"

/**
 * @struct HTTPServer
 * @brief Represents an HTTP server capable of handling HTTP requests and serving responses.
 * 
 * This structure extends the generic Server structure, allowing for the registration of
 * HTTP routes and the handling of HTTP methods. It also provides functions for launching
 * the server to listen for incoming connections.
 */
struct HTTPServer
{
    struct Server server;        /**< A generic Server object for managing network connections. */
    struct Dictionary routes;    /**< A dictionary of registered routes with URL paths as keys. */

    /* PUBLIC MEMBER METHODS */
    
    /**
     * @brief Registers a URL route with the HTTP server.
     * 
     * This method associates a URL path with a function that handles HTTP requests sent to that path.
     * The function can be associated with multiple HTTP methods.
     * 
     * @param server The HTTPServer structure managing the server.
     * @param route_function The function that will handle requests to the registered URL.
     * @param uri The URL path to be associated with the function.
     * @param num_methods The number of HTTP methods to associate with the route.
     * @param ... A variable number of HTTP methods (from enum HTTPMethods) for the route.
     */
    void (*register_routes)(struct HTTPServer *server, char * (*route_function)(struct HTTPServer *server, struct HTTPRequest *request), char *uri, int num_methods, ...);

    /**
     * @brief Starts the server and begins handling incoming connections.
     * 
     * This method initiates the HTTP server’s infinite loop where it listens for incoming client
     * connections and processes HTTP requests by dispatching them to the appropriate route handlers.
     * 
     * @param server The HTTPServer structure managing the server.
     */
    void (*launch)(struct HTTPServer *server);
};

/**
 * @enum HTTPMethods
 * @brief Enum listing the various HTTP methods supported by the HTTP server.
 * 
 * This enum provides easy reference to common HTTP methods used for handling client requests.
 */
enum HTTPMethods
{
    CONNECT,   /**< The HTTP CONNECT method, typically used for tunneling. */
    DELETE,    /**< The HTTP DELETE method for deleting resources. */
    GET,       /**< The HTTP GET method for retrieving resources. */
    HEAD,      /**< The HTTP HEAD method for retrieving resource headers. */
    OPTIONS,   /**< The HTTP OPTIONS method for querying supported methods. */
    PATCH,     /**< The HTTP PATCH method for partially modifying resources. */
    POST,      /**< The HTTP POST method for submitting data to be processed. */
    PUT,       /**< The HTTP PUT method for creating or updating resources. */
    TRACE      /**< The HTTP TRACE method for performing a message loop-back test. */
};

/**
 * @brief Constructs a new HTTPServer object.
 * 
 * This function initializes an HTTPServer structure, preparing it to register routes
 * and handle HTTP requests. The server is set up with basic HTTP capabilities.
 * 
 * @return An initialized HTTPServer structure ready for use.
 */
struct HTTPServer http_server_constructor(void);

/**
 * @brief Combines the contents of multiple files into a single string.
 * 
 * This function is used to generate web pages from multiple template files by
 * concatenating their contents into a single string for serving to the client.
 * 
 * @param num_templates The number of template files to combine.
 * @param ... A variable number of file paths for the templates.
 * @return A string containing the combined contents of the template files.
 */
char *render_template(int num_templates, ...);

#endif // HTTPSERVER_H
