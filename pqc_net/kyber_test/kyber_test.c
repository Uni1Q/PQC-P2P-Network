//
// Created by rokas on 24/09/2024.
//

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include <oqs/oqs.h>

#if OQS_USE_PTHREADS
#include <pthread.h>
#endif

#ifdef OQS_ENABLE_TEST_CONSTANT_TIME
#include <valgrind/memcheck.h>
#define OQS_TEST_CT_CLASSIFY(addr, len)  VALGRIND_MAKE_MEM_UNDEFINED(addr, len)
#define OQS_TEST_CT_DECLASSIFY(addr, len)  VALGRIND_MAKE_MEM_DEFINED(addr, len)
#else
#define OQS_TEST_CT_CLASSIFY(addr, len)
#define OQS_TEST_CT_DECLASSIFY(addr, len)
#endif

/* Displays hexadecimal strings */
static void OQS_print_hex_string(const char *label, const uint8_t *str, size_t len) {
	printf("%-20s (%4zu bytes):  ", label, len);
	for (size_t i = 0; i < len; i++) {
		printf("%02X", str[i]);
	}
	printf("\n");
}

/* Structure to hold magic numbers for memory checks */
typedef struct magic_s {
	uint8_t val[31];
} magic_t;

/* Function to perform correctness test and measure performance */
static OQS_STATUS kem_test_correctness(const char *method_name, int iterations,
									   double *avg_keygen_time,
									   double *avg_encaps_time,
									   double *avg_decaps_time) {

	OQS_KEM *kem = NULL;
	uint8_t *public_key_alloc = NULL;
	uint8_t *secret_key_alloc = NULL;
	uint8_t *ciphertext_alloc = NULL;
	uint8_t *shared_secret_e_alloc = NULL;
	uint8_t *shared_secret_d_alloc = NULL;

	uint8_t *public_key = NULL;
	uint8_t *secret_key = NULL;
	uint8_t *ciphertext = NULL;
	uint8_t *shared_secret_e = NULL;
	uint8_t *shared_secret_d = NULL;

	OQS_STATUS rc, ret = OQS_ERROR;
	int rv;

	// Initialize magic numbers for memory checks
	magic_t magic;
	OQS_randombytes(magic.val, sizeof(magic_t));

	// Initialize the KEM
	kem = OQS_KEM_new(method_name);
	if (kem == NULL) {
		fprintf(stderr, "ERROR: OQS_KEM_new failed\n");
		goto err;
	}

	printf("================================================================================\n");
	printf("Sample computation for KEM %s\n", kem->method_name);
	printf("================================================================================\n");

	// Allocate memory using standard malloc
	public_key_alloc = malloc(kem->length_public_key + 2 * sizeof(magic_t));
	secret_key_alloc = malloc(kem->length_secret_key + 2 * sizeof(magic_t));
	ciphertext_alloc = malloc(kem->length_ciphertext + 2 * sizeof(magic_t));
	shared_secret_e_alloc = malloc(kem->length_shared_secret + 2 * sizeof(magic_t));
	shared_secret_d_alloc = malloc(kem->length_shared_secret + 2 * sizeof(magic_t));

	if ((public_key_alloc == NULL) || (secret_key_alloc == NULL) ||
		(ciphertext_alloc == NULL) || (shared_secret_e_alloc == NULL) ||
		(shared_secret_d_alloc == NULL)) {
		fprintf(stderr, "ERROR: malloc failed\n");
		goto err;
	}

	// Set the magic numbers before the allocated memory
	memcpy(public_key_alloc, magic.val, sizeof(magic_t));
	memcpy(secret_key_alloc, magic.val, sizeof(magic_t));
	memcpy(ciphertext_alloc, magic.val, sizeof(magic_t));
	memcpy(shared_secret_e_alloc, magic.val, sizeof(magic_t));
	memcpy(shared_secret_d_alloc, magic.val, sizeof(magic_t));

	// Adjust pointers to skip the magic numbers
	public_key = public_key_alloc + sizeof(magic_t);
	secret_key = secret_key_alloc + sizeof(magic_t);
	ciphertext = ciphertext_alloc + sizeof(magic_t);
	shared_secret_e = shared_secret_e_alloc + sizeof(magic_t);
	shared_secret_d = shared_secret_d_alloc + sizeof(magic_t);

	// Set the magic numbers after the allocated memory
	memcpy(public_key + kem->length_public_key, magic.val, sizeof(magic_t));
	memcpy(secret_key + kem->length_secret_key, magic.val, sizeof(magic_t));
	memcpy(ciphertext + kem->length_ciphertext, magic.val, sizeof(magic_t));
	memcpy(shared_secret_e + kem->length_shared_secret, magic.val, sizeof(magic_t));
	memcpy(shared_secret_d + kem->length_shared_secret, magic.val, sizeof(magic_t));

	// Variables to accumulate total time
	double total_keygen_time = 0.0;
	double total_encaps_time = 0.0;
	double total_decaps_time = 0.0;

	// Iterate for the specified number of iterations
	for (int i = 0; i < iterations; i++) {
		struct timespec start, end;
		double elapsed;

		// Measure Key Generation Time
		clock_gettime(CLOCK_MONOTONIC, &start);
		rc = OQS_KEM_keypair(kem, public_key, secret_key);
		clock_gettime(CLOCK_MONOTONIC, &end);
		if (rc != OQS_SUCCESS) {
			fprintf(stderr, "ERROR: OQS_KEM_keypair failed\n");
			goto err;
		}
		elapsed = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3; // µs
		total_keygen_time += elapsed;

		// Measure Encapsulation Time
		clock_gettime(CLOCK_MONOTONIC, &start);
		rc = OQS_KEM_encaps(kem, ciphertext, shared_secret_e, public_key);
		clock_gettime(CLOCK_MONOTONIC, &end);
		if (rc != OQS_SUCCESS) {
			fprintf(stderr, "ERROR: OQS_KEM_encaps failed\n");
			goto err;
		}
		elapsed = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3; // µs
		total_encaps_time += elapsed;

		// Measure Decapsulation Time
		clock_gettime(CLOCK_MONOTONIC, &start);
		rc = OQS_KEM_decaps(kem, shared_secret_d, ciphertext, secret_key);
		clock_gettime(CLOCK_MONOTONIC, &end);
		if (rc != OQS_SUCCESS) {
			fprintf(stderr, "ERROR: OQS_KEM_decaps failed\n");
			goto err;
		}
		elapsed = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3; // µs
		total_decaps_time += elapsed;

		// Verify shared secrets are equal
		if (memcmp(shared_secret_e, shared_secret_d, kem->length_shared_secret) != 0) {
			fprintf(stderr, "ERROR: shared secrets are not equal on iteration %d\n", i + 1);
			OQS_print_hex_string("shared_secret_e", shared_secret_e, kem->length_shared_secret);
			OQS_print_hex_string("shared_secret_d", shared_secret_d, kem->length_shared_secret);
			goto err;
		}
	}

	// Calculate average times
	*avg_keygen_time = total_keygen_time / iterations;
	*avg_encaps_time = total_encaps_time / iterations;
	*avg_decaps_time = total_decaps_time / iterations;

	ret = OQS_SUCCESS;
	goto cleanup;

err:
	ret = OQS_ERROR;

cleanup:
	if (secret_key_alloc) {
		free(secret_key_alloc);
	}
	if (shared_secret_e_alloc) {
		free(shared_secret_e_alloc);
	}
	if (shared_secret_d_alloc) {
		free(shared_secret_d_alloc);
	}
	if (public_key_alloc) {
		free(public_key_alloc);
	}
	if (ciphertext_alloc) {
		free(ciphertext_alloc);
	}
	if (kem) {
		OQS_KEM_free(kem);
	}

	return ret;
}

#ifdef OQS_ENABLE_TEST_CONSTANT_TIME
static void TEST_KEM_randombytes(uint8_t *random_array, size_t bytes_to_read) {
	OQS_randombytes_switch_algorithm("system");
	OQS_randombytes(random_array, bytes_to_read);
	OQS_randombytes_custom_algorithm(&TEST_KEM_randombytes);
	OQS_TEST_CT_CLASSIFY(random_array, bytes_to_read);
}
#endif

#if OQS_USE_PTHREADS
struct thread_data {
	char *alg_name;
	OQS_STATUS rc;
};

void *test_wrapper(void *arg) {
	struct thread_data *td = arg;
	td->rc = kem_test_correctness(td->alg_name, 1000, NULL, NULL, NULL); // Modify if needed
	return NULL;
}
#endif

int main(int argc, char **argv) {
	// Initialize liboqs
	OQS_init();

	printf("Testing KEM algorithms using liboqs version %s\n", OQS_version());

	// Hardcode the algorithm to Kyber512
	const char *alg_name = OQS_KEM_alg_kyber_512;

	// Check if Kyber512 is enabled
	if (!OQS_KEM_alg_is_enabled(alg_name)) {
		printf("KEM algorithm %s not enabled!\n", alg_name);
		OQS_destroy();
		return EXIT_FAILURE;
	}

#ifdef OQS_ENABLE_TEST_CONSTANT_TIME
	OQS_randombytes_custom_algorithm(&TEST_KEM_randombytes);
#else
	OQS_randombytes_switch_algorithm("system");
#endif

	// Number of iterations for performance measurement
	int iterations = 1000;

	// Variables to store average times
	double avg_keygen = 0.0;
	double avg_encaps = 0.0;
	double avg_decaps = 0.0;

	OQS_STATUS rc;

#if OQS_USE_PTHREADS
#define MAX_LEN_KEM_NAME_ 64
	// Don't run certain KEMs in threads due to large stack usage
	char no_thread_kem_patterns[][MAX_LEN_KEM_NAME_]  = {"Classic-McEliece", "HQC-256-"};
	int test_in_thread = 1;
	for (size_t i = 0 ; i < sizeof(no_thread_kem_patterns) / sizeof(no_thread_kem_patterns[0]); ++i) {
		if (strstr(alg_name, no_thread_kem_patterns[i]) != NULL) {
			test_in_thread = 0;
			break;
		}
	}
	if (test_in_thread) {
		pthread_t thread;
		struct thread_data td;
		td.alg_name = (char *)alg_name;  // Cast to non-const
		int trc = pthread_create(&thread, NULL, test_wrapper, &td);
		if (trc) {
			fprintf(stderr, "ERROR: Creating pthread\n");
			OQS_destroy();
			return EXIT_FAILURE;
		}
		pthread_join(thread, NULL);
		rc = td.rc;
	} else {
		rc = kem_test_correctness(alg_name, iterations, &avg_keygen, &avg_encaps, &avg_decaps);
	}
#else
	rc = kem_test_correctness(alg_name, iterations, &avg_keygen, &avg_encaps, &avg_decaps);
#endif

	if (rc != OQS_SUCCESS) {
		OQS_destroy();
		return EXIT_FAILURE;
	}

#if !OQS_USE_PTHREADS
	printf("Kyber %s Speed Test over %d iterations:\n", alg_name, iterations);
	printf("Average Key Generation Time: %.2f µs\n", avg_keygen);
	printf("Average Encapsulation Time: %.2f µs\n", avg_encaps);
	printf("Average Decapsulation Time: %.2f µs\n", avg_decaps);
#endif

	OQS_destroy();
	return EXIT_SUCCESS;
}
