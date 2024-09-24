//
// Created by rokas on 24/09/2024.
//

// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>

extern pthread_mutex_t print_mutex;

void safe_print(const char *format, ...);
void trim_newline(char *str);

#endif // UTILS_H
