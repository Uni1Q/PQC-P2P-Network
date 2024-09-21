/**
 * @file peertopeer.h
 * @brief Header file defining the PeerToPeer structure and related methods for peer-to-peer network communication.
 * 
 * This header provides an interface for creating and managing a peer-to-peer (P2P) connection
 * using both server and client functionalities.
 * 
 * @date 02/09/2024
 * @author rokas
 */

#ifndef PEERTOPEER_H
#define PEERTOPEER_H

#include "Server.h"
#include "../../DataStructures/Lists/LinkedList.h"

/**
 * @struct PeerToPeer
 * @brief Represents a peer-to-peer connection, which includes both server and client functionalities.
 * 
 * This structure holds information about the server and known peer hosts,
 * and includes functions to manage peer-to-peer communication.
 */
struct PeerToPeer
{
    struct Server server;          /**< The server component for handling incoming connections. */
    struct LinkedList known_hosts; /**< A list of known hosts in the peer-to-peer network. */

    int domain;      /**< The domain of the connection, such as AF_INET for IPv4. */
    int service;     /**< The service type, typically SOCK_STREAM for TCP. */
    int protocol;    /**< The protocol to be used, like IPPROTO_TCP for TCP connections. */
    int port;        /**< The port number on which the peer-to-peer connection will be made. */
    long interface;  /**< The IP address or interface on which to bind, e.g., INADDR_ANY. */

    /**
     * @brief Launches the server and client applications in the peer-to-peer network.
     * 
     * This function is responsible for initializing and running the user portal for
     * both server and client operations in a peer-to-peer communication system.
     * 
     * @param peer_to_peer The PeerToPeer structure pointer representing the P2P instance.
     */
    void (*user_portal)(struct PeerToPeer *peer_to_peer);

    /**
     * @brief Defines the server-side functionality in the peer-to-peer connection.
     * 
     * This function is executed on the server side and handles incoming peer requests.
     * 
     * @param arg Argument passed to the server function (typically the peer-to-peer instance).
     * @return A pointer to the result of the server function.
     */
    void *(*server_function)(void *arg);

    /**
     * @brief Defines the client-side functionality in the peer-to-peer connection.
     * 
     * This function is executed on the client side to initiate communication with other peers.
     * 
     * @param arg Argument passed to the client function (typically the peer-to-peer instance).
     * @return A pointer to the result of the client function.
     */
    void *(*client_function)(void *arg);
};

/**
 * @brief Constructs a new PeerToPeer object.
 * 
 * This function initializes a new PeerToPeer structure with the provided parameters.
 * It sets up both the server and client functions required for peer-to-peer communication.
 * 
 * @param domain The domain of the connection, such as AF_INET for IPv4.
 * @param service The service type, typically SOCK_STREAM for TCP.
 * @param protocol The protocol to be used, like IPPROTO_TCP for TCP connections.
 * @param port The port number on which the peer-to-peer communication will occur.
 * @param interface The IP address or interface on which to bind, e.g., INADDR_ANY.
 * @param server_function The function pointer to define server-side behavior.
 * @param client_function The function pointer to define client-side behavior.
 * @return A PeerToPeer structure initialized with the provided values.
 */
struct PeerToPeer peer_to_peer_constructor(int domain, int service, int protocol, int port, long interface, void *(*server_function)(void *arg), void *(*client_function)(void *arg));

#endif //PEERTOPEER_H
