//
// Created by rokas on 04/09/2024.
//

#include "BlockHeaders.h"
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <arpa/inet.h>


struct Block *block_constructor(void *data, unsigned long size)
{
    // Allocate memory for the block headers + data
    struct Block *block = calloc(1, sizeof(struct BlockHeaders) + size);
    if (block == NULL) {
        perror("calloc");
        return NULL;
    }

    // Set block headers (metadata)
    for (int i = 0; i < 64; i++) {
        block->headers.previous[i] = 'a';
        block->headers.by[i] = 'b';
    }
    block->headers.nonce = 1000;
    block->headers.size = size;

    printf("Block size (data): %lu\n", size);

    // Copy the provided data into the block's data field
   // Copy the provided data into the block's data field
memcpy(((unsigned char *)block) + sizeof(struct BlockHeaders), data, size);

printf("Sending block of total size: %lu\n", sizeof(struct BlockHeaders) + size);
printf("Size of BlockHeaders: %lu\n", sizeof(struct BlockHeaders));



    return block;
}

