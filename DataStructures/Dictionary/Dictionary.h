//
// Created by rokas on 02/09/2024.
//

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "../Trees/BinarySearchTree.h"
#include "../Lists/LinkedList.h"
#include "Entry.h"

// The dictionary is a collection of entries stored in a binary search tree
struct Dictionary
{
    // Inherit the binary search tree object
    struct BinarySearchTree binary_search_tree;
    // A linked list to store the dictionary keys for easy iteration
    struct LinkedList keys;

    // Public member methods

    // Inserts and adds new items to dictionary, the user needs only to specify the key, value and their sizes
    void (*insert)(struct Dictionary *dictionary, void *key, unsigned long key_size, void *value, unsigned long value_size);
    // Search looks for a given key in the dictionary and returns its value if found or NULL if not
    void * (*search)(struct Dictionary *dictionary, void *key, unsigned long key_size);
};

// The constructor for a dictionary requires a compare function. This will be passed on to the binary search tree
struct Dictionary dictionary_constructor(int (*compare)(void *entry_one, void *entry_two));
// The destructor will need to free every node in the tree
void dictionary_destructor(struct Dictionary *dictionary);

int compare_string_keys(void *entry_one, void *entry_two);

#endif //DICTIONARY_H
