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

double get_elapsed_time_ms(struct timespec start, struct timespec end);
int send_register_command(int server_sock);
int receive_register_response(int server_sock);
void test_routing_server_communication();
void test_p2p_message_metrics(const char *peer_ip, int peer_port);
int send_single_message(int peer_sock, const char *message, size_t message_len, double *message_time_ms);
void calculate_and_print_throughput(size_t total_bytes_sent, double total_time_ms);

// Calculate Elapsed Time in Milliseconds
double get_elapsed_time_ms(struct timespec start, struct timespec end) {
	double elapsed_sec = (double)(end.tv_sec - start.tv_sec) * 1000.0;        // seconds to milliseconds
	double elapsed_nsec = (double)(end.tv_nsec - start.tv_nsec) / 1e6;          // nanoseconds to milliseconds
	return elapsed_sec + elapsed_nsec;
}

// Function to Send REGISTER Command
int send_register_command(int server_sock) {
	char register_command[BUFFER_SIZE];
	snprintf(register_command, sizeof(register_command), "REGISTER test_client %d\n", 7000);
	if (write(server_sock, register_command, strlen(register_command)) <= 0) {
		perror("write");
		return -1;
	}
	return 0;
}

// Function to Receive REGISTER Response
int receive_register_response(int server_sock) {
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read = read(server_sock, buffer, sizeof(buffer) - 1);
	if (bytes_read <= 0) {
		perror("read");
		return -1;
	}
	buffer[bytes_read] = '\0';
	printf("Received from server: %s", buffer);
	return 0;
}

// Function to Test Communication with Routing Server
void test_routing_server_communication() {
	printf("Connecting to Routing Server at %s:%d...\n", ROUTING_SERVER_IP, ROUTING_SERVER_PORT);
	int server_sock = connect_to_server(ROUTING_SERVER_IP, ROUTING_SERVER_PORT);
	if (server_sock < 0) {
		fprintf(stderr, "Failed to connect to routing server.\n");
		return;
	}

	// Measure time to send REGISTER command and receive response
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	if (send_register_command(server_sock) != 0) {
		close(server_sock);
		return;
	}

	if (receive_register_response(server_sock) != 0) {
		close(server_sock);
		return;
	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);

	double time_taken = get_elapsed_time_ms(start_time, end_time);

	printf("Time to communicate with routing server: %.3f ms\n", time_taken);

	close(server_sock);
}

// send_single_message expects an "ACK" from the peer
int send_single_message(int peer_sock, const char *message, size_t message_len, double *message_time_ms) {
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	ssize_t bytes_sent = write(peer_sock, message, message_len);
	if (bytes_sent <= 0) {
		perror("write");
		return -1;
	}

	// Wait for acknowledgment
	char ack_buffer[BUFFER_SIZE];
	ssize_t bytes_read = read(peer_sock, ack_buffer, sizeof(ack_buffer) - 1);
	if (bytes_read <= 0) {
		perror("read ACK");
		return -1;
	}
	ack_buffer[bytes_read] = '\0';

	// Verify acknowledgment
	if (strncmp(ack_buffer, "ACK", 3) != 0) {
		fprintf(stderr, "Invalid acknowledgment received: %s\n", ack_buffer);
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);
	*message_time_ms = get_elapsed_time_ms(start_time, end_time);

	return 0;
}


// Function to Calculate and Print Throughput
void calculate_and_print_throughput(size_t total_bytes_sent, double total_time_ms) {
	double total_time_sec = total_time_ms / 1000.0;
	double throughput_kbps = (total_bytes_sent / 1024.0) / total_time_sec;
	printf("Sent %zu bytes in %.3f ms\n", total_bytes_sent, total_time_ms);
	printf("Average Throughput: %.3f KB/s\n", throughput_kbps);
}

// Function to Test P2P Message Metrics
void test_p2p_message_metrics(const char *peer_ip, int peer_port) {
	printf("Connecting to Peer at %s:%d...\n", peer_ip, peer_port);
	int peer_sock = connect_to_peer(peer_ip, peer_port);
	if (peer_sock < 0) {
		fprintf(stderr, "Failed to connect to peer at %s:%d\n", peer_ip, peer_port);
		return;
	}

	// Prepare test message
	char *test_message = malloc(TEST_MESSAGE_SIZE);
	if (test_message == NULL) {
		perror("malloc");
		close(peer_sock);
		return;
	}
	memset(test_message, 'A', TEST_MESSAGE_SIZE - 1);
	test_message[TEST_MESSAGE_SIZE - 1] = '\0';

	// Measure total time to send all messages
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	size_t total_bytes_sent = 0;
	double total_message_time_ms = 0.0;

	for (int i = 0; i < NUM_TEST_MESSAGES; i++) {
		double message_time_ms = 0.0;
		if (send_single_message(peer_sock, test_message, TEST_MESSAGE_SIZE, &message_time_ms) != 0) {
			fprintf(stderr, "Failed to send message %d\n", i + 1);
			break;
		}
		total_bytes_sent += TEST_MESSAGE_SIZE;
		total_message_time_ms += message_time_ms;
	}

	clock_gettime(CLOCK_MONOTONIC, &end_time);

	double total_time_taken_ms = get_elapsed_time_ms(start_time, end_time);

	printf("Sent %zu bytes to peer in %.3f ms\n", total_bytes_sent, total_time_taken_ms);
	printf("Average Message Send Time: %.3f ms\n", (double)total_message_time_ms / NUM_TEST_MESSAGES);
	calculate_and_print_throughput(total_bytes_sent, total_time_taken_ms);

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

	printf("\nTesting P2P message metrics with peer %s:%d...\n", peer_ip, peer_port);
	test_p2p_message_metrics(peer_ip, peer_port);

	return EXIT_SUCCESS;
}
