//
// Created by rokas on 24/09/2024.
//

// test/get_peer_list.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../common/constants.h"
#include "../common/network.h"
#include "../common/utils.h"

#define ROUTING_SERVER_IP "159.89.248.152"
#define ROUTING_SERVER_PORT 5453
#define TEST_PEER_USERNAME "test_client"
#define TEST_PEER_PORT 7000
#define BUFFER_SIZE 1024

int main() {
    // Connect to the routing server
    int server_sock = connect_to_server(ROUTING_SERVER_IP, ROUTING_SERVER_PORT);
    if (server_sock < 0) {
        fprintf(stderr, "Failed to connect to routing server.\n");
        exit(EXIT_FAILURE);
    }

    // Send REGISTER command
    char register_command[BUFFER_SIZE];
    snprintf(register_command, sizeof(register_command), "REGISTER %s %d\n", TEST_PEER_USERNAME, TEST_PEER_PORT);
    if (write(server_sock, register_command, strlen(register_command)) <= 0) {
        perror("write");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Read server response
    char response[BUFFER_SIZE];
    ssize_t bytes_read = read(server_sock, response, sizeof(response) - 1);
    if (bytes_read <= 0) {
        perror("read");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    response[bytes_read] = '\0';

    if (strncmp(response, "USERNAME_TAKEN", 14) == 0) {
        fprintf(stderr, "Username '%s' is already taken.\n", TEST_PEER_USERNAME);
        close(server_sock);
        exit(EXIT_FAILURE);
    } else if (strncmp(response, "INVALID_COMMAND", 15) == 0) {
        fprintf(stderr, "Invalid REGISTER command format.\n");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Registered with routing server as '%s' on port %d.\n", TEST_PEER_USERNAME, TEST_PEER_PORT);

    // Send GET_PEER_LIST command
    char get_peer_list_cmd[] = "GET_PEER_LIST\n";
    if (write(server_sock, get_peer_list_cmd, strlen(get_peer_list_cmd)) <= 0) {
        perror("write");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Read response from the server
    bytes_read = read(server_sock, response, sizeof(response) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    response[bytes_read] = '\0';

    // Display the peer list
    printf("Peer List:\n%s\n", response);

    // Close the connection
    close(server_sock);
    return 0;
}
