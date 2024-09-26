//
// Created by rokas on 24/09/2024.
//

#include "network.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

// Function to create a socket
int create_socket() {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
	}
	return sock;
}

// Connect to the routing server
int connect_to_server(const char *ip, int port) {
	int sock = create_socket();
	if (sock < 0) return -1;

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	// Convert IP address from string to binary form
	if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
		perror("inet_pton");
		close(sock);
		return -1;
	}

	// Connect to the server
	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect");
		close(sock);
		return -1;
	}

	return sock;
}

// Bind and listen on a given port
int bind_and_listen(int port) {
	int server_sock = create_socket();
	if (server_sock < 0) return -1;

	int opt = 1;
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
				   &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		close(server_sock);
		return -1;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface

	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		close(server_sock);
		return -1;
	}

	if (listen(server_sock, 10) < 0) {
		perror("listen");
		close(server_sock);
		return -1;
	}

	return server_sock;
}

// Function to connect to a peer
int connect_to_peer(const char *ip, int port) {
	int sock = create_socket();
	if (sock < 0) return -1;

	struct sockaddr_in peer_addr;
	memset(&peer_addr, 0, sizeof(peer_addr));
	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(port);

	// Convert IP address from string to binary form
	if (inet_pton(AF_INET, ip, &peer_addr.sin_addr) <= 0) {
		perror("inet_pton");
		close(sock);
		return -1;
	}

	// Connect to the peer
	if (connect(sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0) {
		perror("connect");
		close(sock);
		return -1;
	}

	return sock;
}
