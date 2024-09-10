//
// Created by rokas on 04/09/2024.
//

#include "BlockHeaders.h"
#include <stdlib.h>
#include <string.h>


struct Block * block_constructor(void *data, unsigned long size)
{
    struct Block *block = calloc(1, sizeof(struct BlockHeaders) + size);


    for (int i = 0; i < 64; i++)
    {
        block->headers.previous[i] = 'a';
        block->headers.by[i] = 'b';
    }
    block->headers.nonce = 1000;
    block->headers.size = size + sizeof(struct BlockHeaders);

    memcpy(&block->data, data, size);
    return block;
}