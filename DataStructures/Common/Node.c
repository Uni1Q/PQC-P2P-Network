//
// Created by rokas on 02/09/2024.
//
#include "Node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Constructor used to create new instances of nodes
struct Node node_constructor(void *data, unsigned long size)
{
    if (size < 1)
    {
        // Confirm size of data is at least 1
        printf("Invalid data size for node...\n");
        exit(1);
    }
    // Create node instance
    struct Node node;
    // Allocate space for data if its supported
    node.data = malloc(size);
    memcpy(node.data, data, size);
    // Initialize the pointers
    node.next = NULL;
    node.previous = NULL;
    return node;
}

// Removes node by freeing nodes data
void node_destructor(struct Node *node)
{
    free(node->data);
    free(node);
}