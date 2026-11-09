#include <sys/queue.h>
#include <pthread.h>
#include <stdlib.h>

#ifndef QUEUE_H
#define QUEUE_H

#define LINE_LENGTH 1024

/**
 * Queue node
 */
struct node
{
    char data[LINE_LENGTH];
    TAILQ_ENTRY(node)
    entries;
};

// Define the queue head type
TAILQ_HEAD(tailhead, node);

typedef struct
{
    struct tailhead head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int is_finished;
} SharedQueue;

/**
 * Finishes a queue
 * @param queue The queue to be finished
 */
void finish_queue(SharedQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    queue->is_finished = 1;
    pthread_cond_broadcast(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

/**
 * Starts a queue
 * @param queue The queue struct to be started
 */
void start_queue(SharedQueue *queue)
{
    // Initialize reader queue
    TAILQ_INIT(&queue->head);
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    queue->is_finished = 0;
}

/**
 * Inserts an element into a queue
 * @param queue The queue to be inserted into
 * @param element The element to insert
 */
void insert_element(SharedQueue *queue, struct node *element)
{
    // Lock the queue
    pthread_mutex_lock(&queue->mutex);

    // Insert the node into the queue
    TAILQ_INSERT_TAIL(&queue->head, element, entries);

    pthread_cond_signal(&queue->cond);

    // Unlock the queue
    pthread_mutex_unlock(&queue->mutex);
}

/**
 * Get an element from the queue
 * @param queue The queue to get the element from
 */
struct node *get_element(SharedQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);

    while (TAILQ_EMPTY(&queue->head) && !queue->is_finished)
        pthread_cond_wait(&queue->cond, &queue->mutex);

    if (TAILQ_EMPTY(&queue->head) && queue->is_finished)
    {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    struct node *element = TAILQ_FIRST(&queue->head);

    TAILQ_REMOVE(&queue->head, element, entries);

    pthread_mutex_unlock(&queue->mutex);

    return element;
}

#endif