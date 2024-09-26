//
// Created by rokas on 25/09/2024.
//

#ifndef PQ_ENCRYPTION_H
#define PQ_ENCRYPTION_H

#include <oqs/oqs.h>
#include <stddef.h>

#define AES_KEY_SIZE 32        // 256 bits
#define AES_GCM_IV_SIZE 12     // 96 bits
#define AES_GCM_TAG_SIZE 16    // 128 bits

// Structure to hold encryption context
typedef struct {
	unsigned char aes_key[AES_KEY_SIZE];
	unsigned char iv[AES_GCM_IV_SIZE];
} encryption_context_t;

// Initialize the OQS library, only called once
void pqcrypto_initialize();

// Generate key pair for Kyber KEM
int pqcrypto_generate_keypair(unsigned char **public_key, size_t *public_key_len,
							  unsigned char **secret_key, size_t *secret_key_len);

// Derive shared secret using Kyber KEM
int pqcrypto_derive_shared_secret(encryption_context_t *enc_ctx,
								  const unsigned char *ciphertext, size_t ciphertext_len,
								  const unsigned char *secret_key);

// Encrypt data using AES-256-GCM
int pqcrypto_encrypt(const encryption_context_t *enc_ctx,
					 const unsigned char *plaintext, size_t plaintext_len,
					 unsigned char *ciphertext, size_t *ciphertext_len);

// Decrypt data using AES-256-GCM
int pqcrypto_decrypt(const encryption_context_t *enc_ctx,
					 const unsigned char *ciphertext, size_t ciphertext_len,
					 unsigned char *plaintext, size_t *plaintext_len);

#endif // PQ_ENCRYPTION_H

