//
// Created by rokas on 04/09/2024.
//

#ifndef BLOCKHEADERS_H
#define BLOCKHEADERS_H

struct BlockHeaders                 //        Bytes:
{
    unsigned char previous[64];     //  0       -       63
    unsigned long nonce;            //  64      -       71
    unsigned char by[64];           //  72      -       135
    char timestamp[20];             //  136     -       155
    unsigned long size;             //  156     -       164
}__attribute__((packed));

struct Block
{
    struct BlockHeaders headers;
    unsigned char data;
}__attribute__((packed));

struct Block * block_constructor(void *data, unsigned long size);

#endif //BLOCKHEADERS_H
