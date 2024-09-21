/**
 * @file thread_pool.h
 * @brief Header file defining the ThreadPool and ThreadJob structures for managing a pool of threads and executing jobs.
 * 
 * This header provides an interface for creating and managing a thread pool
 * with the ability to queue and execute jobs using multiple threads.
 * 
 * @date 02/09/2024
 * @author rokas
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stdlib.h>
#include <pthread.h>
#include "../DataStructures/Lists/Queue.h"

/**
 * @struct ThreadJob
 * @brief Represents a unit of work to be executed by the thread pool.
 * 
 * This structure contains a function pointer representing the job to be executed,
 * and a pointer to the argument that will be passed to the job function.
 */
struct ThreadJob
{
    /**
     * @brief The function to be executed by a thread.
     * 
     * This function will be called with the provided argument when a thread picks
     * up this job from the queue.
     * 
     * @param arg The argument passed to the job function.
     * @return A pointer to the result of the job function.
     */
    void *(*job)(void *arg);

    /**
     * @brief The argument to be passed to the job function.
     */
    void *arg;
};

/**
 * @struct ThreadPool
 * @brief Represents a pool of threads used to execute jobs concurrently.
 * 
 * This structure manages a set of threads that pull jobs from a queue and execute them.
 * It includes mechanisms for thread synchronization and job queue management.
 */
struct ThreadPool
{
    int num_threads;            /**< The number of threads in the pool. */
    int active;                 /**< A control switch to manage the pool’s active state (1 = active, 0 = inactive). */
    struct Queue work;          /**< A queue storing jobs to be processed by the threads. */
    pthread_t *pool;            /**< Array of threads in the pool. */
    pthread_mutex_t lock;       /**< A mutex lock to ensure thread-safe access to the job queue. */
    pthread_cond_t signal;      /**< A condition variable used to signal waiting threads when new work is available. */

    /**
     * @brief Adds a job to the work queue.
     * 
     * This function safely adds a new job to the thread pool's job queue, ensuring that
     * threads are notified when new work is available.
     * 
     * @param thread_pool The ThreadPool structure pointer managing the pool.
     * @param thread_job The ThreadJob to be added to the work queue.
     */
    void (*add_work)(struct ThreadPool *thread_pool, struct ThreadJob thread_job);
};

/**
 * @brief Constructs a new ThreadPool object.
 * 
 * This function initializes a ThreadPool with the specified number of threads
 * and prepares it for accepting and executing jobs.
 * 
 * @param num_threads The number of threads to be created in the thread pool.
 * @return A ThreadPool structure initialized with the specified number of threads.
 */
struct ThreadPool thread_pool_constructor(int num_threads);

/**
 * @brief Constructs a new ThreadJob object.
 * 
 * This function initializes a ThreadJob with the provided job function and its argument.
 * 
 * @param job The function to be executed by the thread.
 * @param arg The argument to be passed to the job function.
 * @return A ThreadJob structure initialized with the job and argument.
 */
struct ThreadJob thread_job_constructor(void *(*job)(void *arg), void *arg);

/**
 * @brief Generic function executed by each thread in the pool.
 * 
 * This function is executed by each thread and is responsible for pulling jobs
 * from the queue and executing them.
 * 
 * @param arg The argument passed to the thread (typically the ThreadPool instance).
 * @return A pointer to the result of the thread’s execution.
 */
void *generic_thread_function(void *arg);

/**
 * @brief Adds a job to the thread pool’s work queue.
 * 
 * This function is an implementation of the thread pool’s add_work method.
 * It safely adds a job to the queue and signals the threads that new work is available.
 * 
 * @param thread_pool The ThreadPool structure pointer managing the pool.
 * @param thread_job The ThreadJob to be added to the queue.
 */
void add_work(struct ThreadPool *thread_pool, struct ThreadJob thread_job);

/**
 * @brief Destroys the thread pool and cleans up resources.
 * 
 * This function gracefully shuts down the thread pool, waits for any active jobs
 * to complete, and frees allocated memory for the threads and other resources.
 * 
 * @param thread_pool The ThreadPool structure pointer to be destroyed.
 */
void thread_pool_destructor(struct ThreadPool *thread_pool);

#endif // THREAD_POOL_H
