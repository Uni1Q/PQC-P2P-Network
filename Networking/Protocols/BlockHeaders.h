/**
 * @file block_headers.h
 * @brief Header file defining the Block and BlockHeaders structures for managing blockchain data blocks.
 * 
 * This header provides the data structures and methods required to create and manage 
 * blocks in a blockchain, including the block headers that store metadata such as 
 * the previous block hash, nonce, creator information, and timestamp.
 * 
 * @date 04/09/2024
 * @author rokas
 */

#ifndef BLOCKHEADERS_H
#define BLOCKHEADERS_H

/**
 * @struct BlockHeaders
 * @brief Represents the metadata (headers) for a block in a blockchain.
 * 
 * This structure contains important metadata related to a specific block,
 * such as the hash of the previous block, a nonce for proof of work, creator details,
 * a timestamp, and the size of the block data.
 * 
 * The structure is packed to ensure no padding is introduced, preserving byte alignment
 * for precise memory layout.
 */
struct BlockHeaders                  //        Bytes:
{
    unsigned char previous[64];      /**< Hash of the previous block (0 - 63). */
    unsigned long nonce;             /**< Nonce used for proof of work (64 - 71). */
    unsigned char by[64];            /**< Identifier of the block creator (72 - 135). */
    char timestamp[20];              /**< Timestamp when the block was created (136 - 155). */
    unsigned long size;              /**< Size of the block data (156 - 164). */
} __attribute__((packed));

/**
 * @struct Block
 * @brief Represents a data block in a blockchain.
 * 
 * This structure encapsulates both the block headers (metadata) and the actual
 * block data. The headers contain metadata while the data field holds the block's contents.
 * The structure is packed to ensure proper byte alignment.
 */
struct Block
{
    struct BlockHeaders headers;     /**< The metadata headers for the block. */
    unsigned char data;              /**< The actual data contained in the block. */
} __attribute__((packed));

/**
 * @brief Constructs a new Block object.
 * 
 * This function initializes a new Block structure with the provided data and size.
 * The block's headers, including nonce, timestamp, and size, are set based on the inputs.
 * 
 * @param data A pointer to the data to be stored in the block.
 * @param size The size of the data in bytes.
 * @return A pointer to the newly constructed Block structure.
 */
struct Block *block_constructor(void *data, unsigned long size);

#endif //BLOCKHEADERS_H
