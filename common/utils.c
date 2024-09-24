//
// Created by rokas on 24/09/2024.
//

// utils.c
#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void safe_print(const char *format, ...) {
    va_list args;
    pthread_mutex_lock(&print_mutex);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    pthread_mutex_unlock(&print_mutex);
}

void trim_newline(char *str) {
    if (str == NULL) return;
    size_t len = strlen(str);
    if (len == 0) return;
    if (str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}
