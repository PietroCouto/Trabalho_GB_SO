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
 * Reader queue node
 */
struct reader_node
{
    char data[LINE_LENGTH];
    TAILQ_ENTRY(reader_node)
    entries;
};

// Define the queue head type
TAILQ_HEAD(tailhead, reader_node);

typedef struct
{
    struct tailhead head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int is_finished;
    char *filepath;
} SharedReaderQueue;

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
    SharedReaderQueue reader_queue;

    /**
     * Prepare the reader queue
     */
    TAILQ_INIT(&reader_queue.head);
    pthread_mutex_init(&reader_queue.mutex, NULL);
    pthread_cond_init(&reader_queue.cond, NULL);
    reader_queue.is_finished = 0;
    reader_queue.filepath = argv[1];

    pthread_t read_thread;
    pthread_t compress_thread;
    pthread_t write_thread;

    pthread_create(&read_thread, NULL, reader, &reader_queue);
    pthread_create(&compress_thread, NULL, compressor, NULL);
    pthread_create(&write_thread, NULL, writer, NULL);

    pthread_join(read_thread, NULL);
    pthread_join(compress_thread, NULL);
    pthread_join(write_thread, NULL);

    return 0;
}

void *reader(void *arg)
{
    // Get the queue structure
    SharedReaderQueue *queue = (SharedReaderQueue *)arg;

    FILE *file;

    // Try to open the file
    file = fopen(queue->filepath, "r");

    // Check for failure
    if (file == NULL)
    {
        queue->is_finished = 1;
        return NULL;
    }

    // Create the string to hold the file lines
    char line[LINE_LENGTH];

    // Start reading
    while (fgets(line, LINE_LENGTH, file))
    {
        struct reader_node *element = malloc(sizeof(struct reader_node));
        strcpy(element->data, line);
        TAILQ_INSERT_TAIL(&queue->head, element, entries);
    }

    queue->is_finished = 1;

    // Close the file
    fclose(file);
};

void *compressor(void *arg) {};

void *writer(void *arg) {
    // Create file

    // Get words from the compressor buffer

    // write to file
};