//
// Created by rokas on 24/09/2024.
//

// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

// Function Prototypes
void safe_fgets(char *buffer, size_t size, FILE *stream);
void trim_newline(char *str);
void safe_print(const char *format, ...);

// Declare print_mutex
extern pthread_mutex_t print_mutex;

#endif // UTILS_H
