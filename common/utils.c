//
// Created by rokas on 24/09/2024.
//

#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

// Initialize the mutex
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Safely get input from the user
void safe_fgets(char *buffer, size_t size, FILE *stream) {
	if (fgets(buffer, size, stream) != NULL) {
		// Successfully read input
	} else {
		buffer[0] = '\0';
	}
}

// Trim the newline character from a string
void trim_newline(char *str) {
	str[strcspn(str, "\n")] = '\0';
}

// Safely print to stdout with mutex protection
void safe_print(const char *format, ...) {
	va_list args;
	va_start(args, format);
	pthread_mutex_lock(&print_mutex);
	vprintf(format, args);
	pthread_mutex_unlock(&print_mutex);
	va_end(args);
}
