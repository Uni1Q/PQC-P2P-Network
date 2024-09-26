//
// Created by rokas on 25/09/2024.
//

#include "pq_encryption.h"  // Must include pq_encryption.h to get OQS declarations
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Initialize the OQS library
void pqcrypto_initialize() {
	OQS_init();
}

// Generate Kyber key pair
int pqcrypto_generate_keypair(unsigned char **public_key, size_t *public_key_len,
							  unsigned char **secret_key, size_t *secret_key_len) {
	OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_kyber_512);
	if (kem == NULL) {
		fprintf(stderr, "Failed to create Kyber KEM object\n");
		return -1;
	}

	// Allocate memory for public and secret keys
	*public_key = malloc(kem->length_public_key);
	*secret_key = malloc(kem->length_secret_key);
	if (*public_key == NULL || *secret_key == NULL) {
		fprintf(stderr, "Failed to allocate memory for key pair\n");
		OQS_KEM_free(kem);
		return -1;
	}

	// Generate key pair
	if (OQS_KEM_keypair(kem, *public_key, *secret_key) != OQS_SUCCESS) {
		fprintf(stderr, "Failed to generate Kyber key pair\n");
		free(*public_key);
		free(*secret_key);
		OQS_KEM_free(kem);
		return -1;
	}

	*public_key_len = kem->length_public_key;
	*secret_key_len = kem->length_secret_key;

	OQS_KEM_free(kem);
	return 0;
}

// Derive shared secret using Kyber KEM
int pqcrypto_derive_shared_secret(encryption_context_t *enc_ctx,
								  const unsigned char *ciphertext, size_t ciphertext_len,
								  const unsigned char *secret_key) {
	OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_kyber_512);
	if (kem == NULL) {
		fprintf(stderr, "Failed to create Kyber KEM object\n");
		return -1;
	}

	unsigned char shared_secret[kem->length_shared_secret];

	// Decapsulate to derive shared secret
	if (OQS_KEM_decaps(kem, shared_secret, ciphertext, secret_key) != OQS_SUCCESS) {
		fprintf(stderr, "Failed to decapsulate shared secret\n");
		OQS_KEM_free(kem);
		return -1;
	}

	// Derive AES key from shared secret
	// Here, we'll use the first 32 bytes (256 bits) as the AES key
	memcpy(enc_ctx->aes_key, shared_secret, AES_KEY_SIZE);

	// Generate random IV
	if (!RAND_bytes(enc_ctx->iv, AES_GCM_IV_SIZE)) {
		fprintf(stderr, "Failed to generate random IV\n");
		OQS_KEM_free(kem);
		return -1;
	}

	OQS_KEM_free(kem);
	return 0;
}

// Encrypt data using AES-256-GCM
int pqcrypto_encrypt(const encryption_context_t *enc_ctx,
					 const unsigned char *plaintext, size_t plaintext_len,
					 unsigned char *ciphertext, size_t *ciphertext_len) {
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		perror("EVP_CIPHER_CTX_new");
		return -1;
	}

	if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
		perror("EVP_EncryptInit_ex");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_IV_SIZE, NULL)) {
		perror("EVP_CIPHER_CTX_ctrl");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, enc_ctx->aes_key, enc_ctx->iv)) {
		perror("EVP_EncryptInit_ex");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	int len;

	if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
		perror("EVP_EncryptUpdate");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	*ciphertext_len = len;

	if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
		perror("EVP_EncryptFinal_ex");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	*ciphertext_len += len;

	// Get the tag
	unsigned char tag[AES_GCM_TAG_SIZE];
	if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_GCM_TAG_SIZE, tag)) {
		perror("EVP_CIPHER_CTX_ctrl");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	// Append the tag to the ciphertext
	memcpy(ciphertext + *ciphertext_len, tag, AES_GCM_TAG_SIZE);
	*ciphertext_len += AES_GCM_TAG_SIZE;

	EVP_CIPHER_CTX_free(ctx);
	return 0;
}

// Decrypt data using AES-256-GCM
int pqcrypto_decrypt(const encryption_context_t *enc_ctx,
					 const unsigned char *ciphertext, size_t ciphertext_len,
					 unsigned char *plaintext, size_t *plaintext_len) {
	if (ciphertext_len < AES_GCM_TAG_SIZE) {
		fprintf(stderr, "Ciphertext too short\n");
		return -1;
	}

	size_t actual_ciphertext_len = ciphertext_len - AES_GCM_TAG_SIZE;
	unsigned char tag[AES_GCM_TAG_SIZE];
	memcpy(tag, ciphertext + actual_ciphertext_len, AES_GCM_TAG_SIZE);

	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		perror("EVP_CIPHER_CTX_new");
		return -1;
	}

	if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
		perror("EVP_DecryptInit_ex");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_IV_SIZE, NULL)) {
		perror("EVP_CIPHER_CTX_ctrl");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	if (1 != EVP_DecryptInit_ex(ctx, NULL, NULL, enc_ctx->aes_key, enc_ctx->iv)) {
		perror("EVP_DecryptInit_ex");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	int len;

	if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, actual_ciphertext_len)) {
		perror("EVP_DecryptUpdate");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	*plaintext_len = len;

	// Set expected tag value
	if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_GCM_TAG_SIZE, tag)) {
		perror("EVP_CIPHER_CTX_ctrl");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}

	// Finalize decryption
	int ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
	if (ret > 0) {
		*plaintext_len += len;
		EVP_CIPHER_CTX_free(ctx);
		return 0;
	} else {
		fprintf(stderr, "Decryption failed: authentication tag mismatch\n");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
}
