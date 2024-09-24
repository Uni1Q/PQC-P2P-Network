// testClient.c
// A test client application to measure the performance of the P2P networking application.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

// Constants
#define BUFFER_SIZE 1024

// Structure to hold performance metrics
typedef struct {
    double registration_time;     // in milliseconds
    double peer_list_time;        // in milliseconds
    double connection_time;       // in milliseconds
    double message_send_time;     // in milliseconds
    double message_receive_time;  // in milliseconds
    size_t total_bytes_sent;
    size_t total_bytes_received;
} PerformanceMetrics;

// Utility function to get current time in milliseconds
double get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)(tv.tv_sec) * 1000.0 + (double)(tv.tv_usec) / 1000.0;
}

// Function to handle registration
int register_with_server(int sock, const char *username, int listen_port, PerformanceMetrics *metrics) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "REGISTER %s %d\n", username, listen_port);

    double start_time = get_current_time_ms();
    ssize_t bytes_sent = write(sock, buffer, strlen(buffer));
    if (bytes_sent == -1) {
        perror("Registration write failed");
        return -1;
    }

    // Wait for server response
    ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        perror("Registration read failed");
        return -1;
    }
    buffer[bytes_read] = '\0';
    double end_time = get_current_time_ms();

    metrics->registration_time = end_time - start_time;

    // Check server response
    if (strncmp(buffer, "USERNAME_TAKEN", 13) == 0) {
        fprintf(stderr, "Username '%s' is already taken.\n", username);
        return -1;
    }

    // Assuming successful registration returns the peer list
    return 0;
}

// Function to retrieve peer list
int get_peer_list(int sock, char *peer_list, size_t size, PerformanceMetrics *metrics) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "GET_PEER_LIST\n");

    double start_time = get_current_time_ms();
    ssize_t bytes_sent = write(sock, buffer, strlen(buffer));
    if (bytes_sent == -1) {
        perror("GET_PEER_LIST write failed");
        return -1;
    }

    // Wait for server response
    ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        perror("GET_PEER_LIST read failed");
        return -1;
    }
    buffer[bytes_read] = '\0';
    double end_time = get_current_time_ms();

    metrics->peer_list_time = end_time - start_time;

    // Copy peer list to provided buffer
    strncpy(peer_list, buffer, size - 1);
    peer_list[size - 1] = '\0';

    return 0;
}

// Function to connect to a peer
int connect_to_peer(const char *peer_ip, int peer_port, const char *username, PerformanceMetrics *metrics) {
    int peer_sock;
    struct sockaddr_in peer_addr;
    char buffer[BUFFER_SIZE];

    peer_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (peer_sock < 0) {
        perror("Peer socket creation failed");
        return -1;
    }

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    if (inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr) <= 0) {
        perror("Invalid peer IP address");
        close(peer_sock);
        return -1;
    }

    double start_time = get_current_time_ms();
    if (connect(peer_sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) < 0) {
        perror("Connecting to peer failed");
        close(peer_sock);
        return -1;
    }
    double end_time = get_current_time_ms();
    metrics->connection_time = end_time - start_time;

    // Send CONNECT_REQUEST with the test client's username
    snprintf(buffer, sizeof(buffer), "CONNECT_REQUEST %s\n", username);

    double send_start = get_current_time_ms();
    ssize_t bytes_sent = write(peer_sock, buffer, strlen(buffer));
    if (bytes_sent == -1) {
        perror("CONNECT_REQUEST write failed");
        close(peer_sock);
        return -1;
    }
    double send_end = get_current_time_ms();
    metrics->message_send_time += (send_end - send_start);
    metrics->total_bytes_sent += bytes_sent;

    // Wait for ACCEPT or DENY
    ssize_t bytes_read = read(peer_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        perror("CONNECT_REQUEST read failed");
        close(peer_sock);
        return -1;
    }
    buffer[bytes_read] = '\0';

    if (strncmp(buffer, "ACCEPT", 6) == 0) {
        printf("Peer accepted the connection.\n");
    } else if (strncmp(buffer, "DENY", 4) == 0) {
        printf("Peer denied the connection.\n");
        close(peer_sock);
        return -1;
    } else {
        printf("Unknown response from peer: %s\n", buffer);
        close(peer_sock);
        return -1;
    }

    return peer_sock;
}

// Function to send messages to peer
int send_messages_to_peer(int peer_sock, int num_messages, int message_size, PerformanceMetrics *metrics) {
    char *message = malloc(message_size);
    if (!message) {
        fprintf(stderr, "Failed to allocate memory for messages.\n");
        return -1;
    }
    memset(message, 'A', message_size - 1);
    message[message_size - 1] = '\0';

    char buffer[BUFFER_SIZE];
    ssize_t bytes_sent, bytes_received;
    double send_start, send_end, receive_start, receive_end;

    for (int i = 0; i < num_messages; i++) {
        // Send message
        send_start = get_current_time_ms();
        bytes_sent = write(peer_sock, message, strlen(message));
        if (bytes_sent == -1) {
            perror("Message write failed");
            free(message);
            return -1;
        }
        send_end = get_current_time_ms();
        metrics->message_send_time += (send_end - send_start);
        metrics->total_bytes_sent += bytes_sent;

        // Receive echo (assuming peer echoes messages)
        receive_start = get_current_time_ms();
        bytes_received = read(peer_sock, buffer, sizeof(buffer) - 1);
        if (bytes_received == -1) {
            perror("Message read failed");
            free(message);
            return -1;
        }
        buffer[bytes_received] = '\0';
        receive_end = get_current_time_ms();
        metrics->message_receive_time += (receive_end - receive_start);
        metrics->total_bytes_received += bytes_received;

        // Optional: Validate echoed message
        if (strncmp(buffer, message, message_size - 1) != 0) {
            printf("Mismatch in echoed message.\n");
        }
    }

    free(message);
    return 0;
}

// Function to parse peer list and select the first available peer
int select_first_peer(const char *peer_list, char *peer_ip, int *peer_port, const char *own_username) {
    // Assuming peer list format: "username ip port\nusername ip port\n..."
    char peer_list_copy[BUFFER_SIZE];
    strncpy(peer_list_copy, peer_list, sizeof(peer_list_copy) - 1);
    peer_list_copy[sizeof(peer_list_copy) - 1] = '\0';

    char *line = strtok(peer_list_copy, "\n");
    while (line != NULL) {
        char username[50];
        char ip[INET_ADDRSTRLEN];
        int port;
        if (sscanf(line, "%s %s %d", username, ip, &port) == 3) {
            // Skip own username
            if (strcmp(username, own_username) != 0) {
                strncpy(peer_ip, ip, INET_ADDRSTRLEN - 1);
                peer_ip[INET_ADDRSTRLEN - 1] = '\0';
                *peer_port = port;
                return 0;
            }
        }
        line = strtok(NULL, "\n");
    }

    fprintf(stderr, "No suitable peers found.\n");
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("Usage: %s <server_ip> <server_port> <username> <listen_port> <num_messages> <message_size>\n", argv[0]);
        printf("Example: %s 127.0.0.1 5453 test_client 5555 100 256\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    const char *username = argv[3];
    int listen_port = atoi(argv[4]);
    int num_messages = atoi(argv[5]);
    int message_size = atoi(argv[6]);

    PerformanceMetrics metrics;
    memset(&metrics, 0, sizeof(metrics));

    // Create socket to connect to routing server
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr_struct;
    server_addr_struct.sin_family = AF_INET;
    server_addr_struct.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr_struct.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to routing server
    if (connect(sock, (struct sockaddr *)&server_addr_struct, sizeof(server_addr_struct)) < 0) {
        perror("Connection to routing server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to routing server at %s:%d\n", server_ip, server_port);

    // Register with server
    if (register_with_server(sock, username, listen_port, &metrics) == -1) {
        fprintf(stderr, "Registration failed.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Registered with server in %.2f ms\n", metrics.registration_time);

    // Retrieve peer list
    char peer_list[BUFFER_SIZE];
    if (get_peer_list(sock, peer_list, sizeof(peer_list), &metrics) == -1) {
        fprintf(stderr, "Failed to retrieve peer list.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Retrieved peer list in %.2f ms\n", metrics.peer_list_time);
    printf("Peer List:\n%s\n", peer_list);

    // Select a peer to connect
    char peer_ip[INET_ADDRSTRLEN];
    int peer_port;
    if (select_first_peer(peer_list, peer_ip, &peer_port, username) == -1) {
        fprintf(stderr, "No peers available for connection.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Selected peer: %s:%d\n", peer_ip, peer_port);

    // Connect to peer
    int peer_sock = connect_to_peer(peer_ip, peer_port, username, &metrics);
    if (peer_sock == -1) {
        fprintf(stderr, "Failed to connect to peer.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected to peer in %.2f ms\n", metrics.connection_time);

    // Send and receive messages
    if (send_messages_to_peer(peer_sock, num_messages, message_size, &metrics) == -1) {
        fprintf(stderr, "Error during message exchange.\n");
        close(peer_sock);
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Sent and received %d messages of size %d bytes.\n", num_messages, message_size);

    // Close peer connection
    close(peer_sock);
    printf("Closed connection to peer.\n");

    // Send REMOVE command to routing server
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "REMOVE %s\n", username);
    ssize_t bytes_sent = write(sock, buffer, strlen(buffer));
    if (bytes_sent == -1) {
        perror("REMOVE command write failed");
    }

    // Close server connection
    close(sock);
    printf("Closed connection to routing server.\n");

    // Display performance metrics
    printf("\n=== Performance Metrics ===\n");
    printf("Registration Time: %.2f ms\n", metrics.registration_time);
    printf("Peer List Retrieval Time: %.2f ms\n", metrics.peer_list_time);
    printf("Connection Time: %.2f ms\n", metrics.connection_time);
    printf("Total Messages Sent: %d\n", num_messages);
    printf("Total Bytes Sent: %zu bytes\n", metrics.total_bytes_sent);
    printf("Total Messages Received: %d\n", num_messages);
    printf("Total Bytes Received: %zu bytes\n", metrics.total_bytes_received);
    printf("Average Message Send Time: %.2f ms\n", (metrics.message_send_time / num_messages));
    printf("Average Message Receive Time: %.2f ms\n", (metrics.message_receive_time / num_messages));

    return 0;
}
