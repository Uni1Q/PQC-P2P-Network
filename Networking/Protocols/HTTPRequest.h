/**
 * @file http_request.h
 * @brief Header file defining the HTTPRequest structure for handling HTTP request data.
 * 
 * This header provides the structures and functions required to parse and manage HTTP requests, 
 * including the request line, header fields, and body data.
 * 
 * @date 02/09/2024
 * @version 1.0
 * @author rokas
 */

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "../../DataStructures/Dictionary/Dictionary.h"

/**
 * @struct HTTPRequest
 * @brief Represents an HTTP request.
 * 
 * This structure is used to store and manage the components of an HTTP request, 
 * which includes the request line, header fields, and body. Each component is stored 
 * in a `Dictionary` for easy lookup and manipulation.
 */
struct HTTPRequest
{
    struct Dictionary request_line;  /**< Dictionary storing the HTTP request line (method, URL, version). */
    struct Dictionary header_fields; /**< Dictionary storing the HTTP header fields (e.g., Content-Type, Host). */
    struct Dictionary body;          /**< Dictionary storing the body of the request, if present. */
};

/**
 * @brief Constructs an HTTPRequest object from a raw HTTP request string.
 * 
 * This function parses a raw HTTP request string into its constituent parts—request line, 
 * headers, and body—and stores them in the respective `Dictionary` fields of the `HTTPRequest` structure.
 * 
 * @param request_string The raw HTTP request string to be parsed.
 * @return An initialized `HTTPRequest` structure with parsed data.
 */
struct HTTPRequest http_request_constructor(char *request_string);

/**
 * @brief Destroys an HTTPRequest object, freeing its associated memory.
 * 
 * This function safely deallocates memory associated with an `HTTPRequest` object, 
 * including the dictionaries for the request line, headers, and body.
 * 
 * @param request A pointer to the `HTTPRequest` object to be destroyed.
 */
void http_request_destructor(struct HTTPRequest *request);

#endif // HTTPREQUEST_H
