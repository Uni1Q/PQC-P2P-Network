//
// Created by rokas on 24/09/2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include "../common/constants.h"
#include "../common/network.h"
#include "../common/utils.h"

#define ROUTING_SERVER_IP "159.89.248.152" // 127.0.0.1 for local testing, 159.89.248.152 to connect to droplet
#define ROUTING_SERVER_PORT 5453
#define TEST_MESSAGE_SIZE 1024
#define NUM_TEST_MESSAGES 1000


// Function to measure time to communicate with routing server
void test_routing_server_communication() {
	int server_sock = connect_to_server(ROUTING_SERVER_IP, ROUTING_SERVER_PORT);
	if (server_sock < 0) {
		fprintf(stderr, "Failed to connect to routing server.\n");
		return;
	}

	// Measure time to send a REGISTER command and receive a response
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	char register_command[BUFFER_SIZE];
	snprintf(register_command, sizeof(register_command), "REGISTER test_client %d\n", 7000);
	if (write(server_sock, register_command, strlen(register_command)) <= 0) {
		perror("write");
		close(server_sock);
		return;
	}

	char buffer[BUFFER_SIZE];
	ssize_t bytes_read = read(server_sock, buffer, sizeof(buffer) - 1);
	if (bytes_read <= 0) {
		perror("read");
		close(server_sock);
		return;
	}
	buffer[bytes_read] = '\0';

	clock_gettime(CLOCK_MONOTONIC, &end_time);

	double time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e3;      // Convert seconds to milliseconds
	time_taken += (end_time.tv_nsec - start_time.tv_nsec) / 1e6;          // Convert nanoseconds to milliseconds

	printf("Time to communicate with routing server: %.3f ms\n", time_taken);

	close(server_sock);
}

// Function to simulate P2P message exchange and collect metrics
void test_p2p_message_metrics(const char *peer_ip, int peer_port) {
	int peer_sock = connect_to_peer(peer_ip, peer_port);
	if (peer_sock < 0) {
		fprintf(stderr, "Failed to connect to peer at %s:%d\n", peer_ip, peer_port);
		return;
	}

	char *test_message = malloc(TEST_MESSAGE_SIZE);
	if (test_message == NULL) {
		perror("malloc");
		close(peer_sock);
		return;
	}
	memset(test_message, 'A', TEST_MESSAGE_SIZE - 1);
	test_message[TEST_MESSAGE_SIZE - 1] = '\0';

	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	size_t total_bytes_sent = 0;
	for (int i = 0; i < NUM_TEST_MESSAGES; i++) {
		ssize_t bytes_sent = write(peer_sock, test_message, TEST_MESSAGE_SIZE);
		if (bytes_sent <= 0) {
			perror("write");
			break;
		}
		total_bytes_sent += bytes_sent;
	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);

	double time_taken = (end_time.tv_sec - start_time.tv_sec) * 1e3;      // Convert seconds to milliseconds
	time_taken += (end_time.tv_nsec - start_time.tv_nsec) / 1e6;          // Convert nanoseconds to milliseconds

	printf("Sent %zu bytes to peer in %.3f ms\n", total_bytes_sent, time_taken);
	printf("Average throughput: %.3f KB/s\n", (total_bytes_sent / 1024.0) / (time_taken / 1000.0));

	free(test_message);
	close(peer_sock);
}

int main(int argc, char *argv[]) {
	printf("Testing communication with routing server...\n");
	test_routing_server_communication();

	if (argc < 3) {
		fprintf(stderr, "Usage: %s <peer_ip> <peer_port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *peer_ip = argv[1];
	int peer_port = atoi(argv[2]);

	printf("Testing P2P message metrics with peer %s:%d...\n", peer_ip, peer_port);
	test_p2p_message_metrics(peer_ip, peer_port);

	return EXIT_SUCCESS;
}
