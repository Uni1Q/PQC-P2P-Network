//
// Created by rokas on 02/09/2024.
//

#ifndef NODE_H
#define NODE_H

struct Node
{
    // data is stored as void pointer - casting is required
    void *data;
    // a pointer next to node
    struct Node *next;
    struct Node *previous;
};

// constructor should be used to create node
struct Node node_constructor(void *data, unsigned long size);
// the destructor should be used to destroy
void node_destructor(struct Node *node);

#endif //NODE_H
