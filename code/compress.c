/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <string.h>

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

typedef struct
{
    SharedQueue *queue;
    char *filepath;
} ReaderStructure;

/**
 * Prototypes
 */
void *reader(void *arg);
void *compressor(void *arg);
void *writer(void *arg);

/**
 * Main function
 * @param argc Argument count
 * @param argv Arguments
 * @returns -1 for error, 0 for success
 */
int main(int argc, char *argv[])
{
    // Create the structure for the reader thread
    ReaderStructure reader_structure;

    // Create the reader FIFO buffer
    SharedQueue reader_queue;

    // Initiate the buffer
    TAILQ_INIT(&reader_queue.head);
    pthread_mutex_init(&reader_queue.mutex, NULL);
    pthread_cond_init(&reader_queue.cond, NULL);

    // Assign the buffer to the reader structure
    reader_structure.queue = &reader_queue;

    // Set is_finished to false
    reader_queue.is_finished = 0;

    // Assign the file path
    reader_structure.filepath = argv[1];

    // Create the thread IDs
    pthread_t read_thread;
    pthread_t compress_thread;
    pthread_t write_thread;

    // Create the threads
    pthread_create(&read_thread, NULL, reader, &reader_structure);
    pthread_create(&compress_thread, NULL, compressor, NULL);
    pthread_create(&write_thread, NULL, writer, NULL);

    // Join the threads
    pthread_join(read_thread, NULL);
    pthread_join(compress_thread, NULL);
    pthread_join(write_thread, NULL);

    return 0;
}

void *reader(void *arg)
{
    // Get the reader structure
    ReaderStructure *reader_structure = (ReaderStructure *)arg;
    SharedQueue *queue = (SharedQueue *)reader_structure->queue;

    FILE *file;

    // Try to open the file
    file = fopen(reader_structure->filepath, "r");

    // Check for failure
    if (file == NULL)
    {
        queue->is_finished = 1;
        return NULL;
    }

    // Create the string to hold the file lines
    char line[LINE_LENGTH];

    // Reading loop
    while (fgets(line, LINE_LENGTH, file))
    {
        // Create the node to hold the data
        struct node *element = malloc(sizeof(struct node));

        // Copy the line into the node
        strcpy(element->data, line);

        // Lock the queue
        pthread_mutex_lock(&queue->mutex);

        // Insert the node into the queue
        TAILQ_INSERT_TAIL(&queue->head, element, entries);

        // Unlock the queue
        pthread_mutex_unlock(&queue->mutex);
    }

    // Lock the queue
    pthread_mutex_lock(&queue->mutex);

    queue->is_finished = 1;

    // Unlock the queue
    pthread_mutex_unlock(&queue->mutex);

    // Close the file
    fclose(file);
};

void *compressor(void *arg) {
    //  struct node *first = TAILQ_FIRST(&queue->head);

    // while (first != NULL) {
    //     printf("Front item: %s\n", first->data);
    //     TAILQ_REMOVE(&queue->head, first, entries);
    //     free(first);

    //     first = TAILQ_FIRST(&queue->head);
    // }
};

void *writer(void *arg) {
    // Create file

    // Get words from the compressor buffer

    // write to file
};