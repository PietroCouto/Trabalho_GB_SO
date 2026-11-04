/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "libs/dictionary.h"

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

typedef struct
{
    SharedQueue *reader_queue;
    SharedQueue *compressor_queue;
} CompressorStructure;

typedef struct
{
    SharedQueue *compressor_queue;
    char *filepath;
} WriterStructure;

/**
 * Prototypes
 */
void *reader(void *arg);
void *compressor(void *arg);
void *writer(void *arg);
void add_char(char *s, char c);

/**
 * Main function
 * @param argc Argument count
 * @param argv Arguments
 * @returns -1 for error, 0 for success
 */
int main(int argc, char *argv[])
{
    // Create the structures for the threads
    ReaderStructure reader_structure;
    CompressorStructure compressor_structure;
    WriterStructure writer_structure;

    // Create the buffers
    SharedQueue reader_queue;
    SharedQueue compressor_queue;

    // Initiate the buffers
    TAILQ_INIT(&reader_queue.head);
    pthread_mutex_init(&reader_queue.mutex, NULL);
    pthread_cond_init(&reader_queue.cond, NULL);
    reader_queue.is_finished = 0;

    TAILQ_INIT(&compressor_queue.head);
    pthread_mutex_init(&compressor_queue.mutex, NULL);
    pthread_cond_init(&compressor_queue.cond, NULL);
    compressor_queue.is_finished = 0;

    // Assign the buffers to the structures
    reader_structure.queue = &reader_queue;

    compressor_structure.reader_queue = &reader_queue;
    compressor_structure.compressor_queue = &compressor_queue;

    writer_structure.compressor_queue = &compressor_queue;

    // Assign the file paths
    reader_structure.filepath = argv[1];
    writer_structure.filepath = argv[2];

    // Create the thread IDs
    pthread_t reader_thread;
    pthread_t compressor_thread;
    pthread_t writer_thread;

    // Create the threads
    pthread_create(&reader_thread, NULL, reader, &reader_structure);
    pthread_create(&compressor_thread, NULL, compressor, &compressor_structure);
    pthread_create(&writer_thread, NULL, writer, &writer_structure);

    // Join the threads
    pthread_join(reader_thread, NULL);
    pthread_join(compressor_thread, NULL);
    pthread_join(writer_thread, NULL);

    return 0;
}

void *reader(void *arg)
{
    // Get the reader structure
    ReaderStructure *reader_structure = (ReaderStructure *)arg;

    // Extract the queue
    SharedQueue *queue = (SharedQueue *)reader_structure->queue;

    // Try to open the file
    FILE *file = fopen(reader_structure->filepath, "r");

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
        struct node *element = calloc(1, sizeof(struct node));

        // Copy the line into the node
        strcpy(element->data, line);

        // Lock the queue
        pthread_mutex_lock(&queue->mutex);

        // Insert the node into the queue
        TAILQ_INSERT_TAIL(&queue->head, element, entries);

        pthread_cond_signal(&queue->cond);

        // Unlock the queue
        pthread_mutex_unlock(&queue->mutex);
    }

    // Lock the queue
    pthread_mutex_lock(&queue->mutex);

    queue->is_finished = 1;

    pthread_cond_broadcast(&queue->cond);

    // Unlock the queue
    pthread_mutex_unlock(&queue->mutex);

    // Close the file
    fclose(file);

    return NULL;
}

/**
 *
 */
void *compressor(void *arg)
{
    CompressorStructure *compressor_structure = (CompressorStructure *)arg;

    // Extract the queues
    SharedQueue *reader_queue = compressor_structure->reader_queue;
    SharedQueue *compressor_queue = compressor_structure->compressor_queue;

    while (true)
    {
        pthread_mutex_lock(&reader_queue->mutex);

        while (TAILQ_EMPTY(&reader_queue->head) && !reader_queue->is_finished)
            pthread_cond_wait(&reader_queue->cond, &reader_queue->mutex);

        if (TAILQ_EMPTY(&reader_queue->head) && reader_queue->is_finished)
        {
            pthread_mutex_unlock(&reader_queue->mutex);
            break;
        }

        struct node *element = TAILQ_FIRST(&reader_queue->head);

        TAILQ_REMOVE(&reader_queue->head, element, entries);

        pthread_mutex_unlock(&reader_queue->mutex);

        // Create the node to hold the compressed data
        struct node *compressed_element = calloc(1, sizeof(struct node));

        int c;      // caractere lido do arquivo
        int wl = 0; // comprimento atual da palavra
        int i = 0;
        char w[256]; // buffer temporário para armazenar uma palavra

        while ((c = element->data[i]) != '\0' && c != '\n')
        {
            if (isalnum(c) || c == '_')
                w[wl++] = c;
            else
            {
                if (wl)
                {
                    w[wl] = 0;
                    wl = 0;
                    int s = word_to_sym(w);

                    if (s >= 0)
                        add_char(compressed_element->data, (char)s);

                    else
                        strcat(compressed_element->data, w);
                }
                add_char(compressed_element->data, (char)c);
            }

            i++;
        }

        if (wl)
        {
            w[wl] = 0;
            int s = word_to_sym(w);
            if (s >= 0)
                add_char(compressed_element->data, (char)s);
            else
                strcat(compressed_element->data, w);
        }

        pthread_mutex_lock(&compressor_queue->mutex);
        TAILQ_INSERT_TAIL(&compressor_queue->head, compressed_element, entries);
        pthread_cond_signal(&compressor_queue->cond);
        pthread_mutex_unlock(&compressor_queue->mutex);

        free(element);
    }

    pthread_mutex_lock(&compressor_queue->mutex);
    compressor_queue->is_finished = 1;
    pthread_cond_broadcast(&compressor_queue->cond);
    pthread_mutex_unlock(&compressor_queue->mutex);

    return NULL;
}

/**
 *
 */
void *writer(void *arg)
{
    WriterStructure *writer_structure = (WriterStructure *)arg;

    // Extract the queue
    SharedQueue *compressor_queue = writer_structure->compressor_queue;

    // Try to open the file
    FILE *file = fopen(writer_structure->filepath, "w");

    // Check for failure
    if (file == NULL)
        return NULL;

    while (true)
    {
        pthread_mutex_lock(&compressor_queue->mutex);

        while (TAILQ_EMPTY(&compressor_queue->head) && !compressor_queue->is_finished)
            pthread_cond_wait(&compressor_queue->cond, &compressor_queue->mutex);

        if (TAILQ_EMPTY(&compressor_queue->head) && compressor_queue->is_finished)
        {
            pthread_mutex_unlock(&compressor_queue->mutex);
            break;
        }

        struct node *element = TAILQ_FIRST(&compressor_queue->head);

        TAILQ_REMOVE(&compressor_queue->head, element, entries);

        pthread_mutex_unlock(&compressor_queue->mutex);

        fprintf(file, "%s\n", element->data);

        fflush(file);
        
        free(element);
    }

    fclose(file);

    return NULL;
}

void add_char(char *s, char c)
{

    // Get the current length of the string
    int l = strlen(s);

    // Add the new character at the end
    s[l] = c;

    // Add the null terminator
    s[l + 1] = '\0';
}
