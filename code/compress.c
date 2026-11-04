/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "libs/dictionary.h"
#include "libs/utils.h"
#include "libs/queue.h"

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
    SharedQueue *queue;
    char *filepath;
} WriterStructure;

/**
 * Prototypes
 */
void *reader(void *arg);
void *compressor(void *arg);
void *writer(void *arg);
void startup_structures(ReaderStructure *reader_structure, CompressorStructure *compressor_structure, WriterStructure *writer_structure, char *input_path, char *output_path);

/**
 * Main function
 * @param argc Argument count
 * @param argv Arguments
 * @returns -1 for error, 0 for success
 */
int main(int argc, char *argv[])
{
    if (argc < 3)
        return return_error("Argumentos insuficientes para o compactador!");

    if (argc > 3)
        return return_error("Argumentos em excesso para o compactador!");

    // Create the structures for the threads
    ReaderStructure reader_structure;
    CompressorStructure compressor_structure;
    WriterStructure writer_structure;

    // Start the structures and queues
    startup_structures(&reader_structure, &compressor_structure, &writer_structure, argv[1], argv[2]);

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

    pthread_mutex_destroy(&reader_structure.queue->mutex);
    pthread_cond_destroy(&reader_structure.queue->cond);
    free(reader_structure.queue);

    pthread_mutex_destroy(&writer_structure.queue->mutex);
    pthread_cond_destroy(&writer_structure.queue->cond);
    free(writer_structure.queue);

    return 0;
}

void startup_structures(ReaderStructure *reader_structure, CompressorStructure *compressor_structure, WriterStructure *writer_structure, char *input_path, char *output_path)
{
    // Create the buffers
    // Allocate the buffers on the heap
    SharedQueue *reader_queue = malloc(sizeof(SharedQueue));
    SharedQueue *compressor_queue = malloc(sizeof(SharedQueue));

    // Initialize queues
    start_queue(reader_queue);
    start_queue(compressor_queue);

    // Assign the heap pointers to the structures
    reader_structure->queue = reader_queue;

    compressor_structure->reader_queue = reader_queue;
    compressor_structure->compressor_queue = compressor_queue;

    writer_structure->queue = compressor_queue;

    // Assign the file paths
    reader_structure->filepath = input_path;
    writer_structure->filepath = output_path;
}

/**
 * Reads a file and writes the lines to the reader buffer
 * @param arg Pointer to the ReaderStructure
 */
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
        // Lock the queue
        finish_queue(queue);
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

        insert_element(queue, element);
    }

    finish_queue(queue);

    // Close the file
    fclose(file);

    return NULL;
}

/**
 * Consumes the reader queue and compresses the lines to the compressor queue
 * @param arg Pointer to the CompressorStructure
 */
void *compressor(void *arg)
{
    CompressorStructure *compressor_structure = (CompressorStructure *)arg;

    // Extract the queues
    SharedQueue *reader_queue = compressor_structure->reader_queue;
    SharedQueue *compressor_queue = compressor_structure->compressor_queue;

    while (true)
    {
        struct node *element = get_element(reader_queue);

        if (element == NULL)
            break;

        // Create the node to hold the compressed data
        struct node *compressed_element = calloc(1, sizeof(struct node));

        int c;      // caractere lido do arquivo
        int wl = 0; // comprimento atual da palavra
        int i = 0;
        char w[256]; // buffer temporário para armazenar uma palavra

        while ((c = element->data[i]) != '\0')
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

        insert_element(compressor_queue, compressed_element);

        free(element);
    }

    finish_queue(compressor_queue);

    return NULL;
}

/**
 * Consumes the compressed queue and writes to the output file
 * @param arg Pointer to the WriterStructure
 */
void *writer(void *arg)
{
    WriterStructure *writer_structure = (WriterStructure *)arg;

    // Extract the queue
    SharedQueue *compressor_queue = writer_structure->queue;

    // Try to open the file
    FILE *file = fopen(writer_structure->filepath, "w");

    // Check for failure
    if (file == NULL)
        return NULL;

    while (true)
    {
        struct node *element = get_element(compressor_queue);

        if (element == NULL)
            break;

        fprintf(file, "%s", element->data);

        fflush(file);

        free(element);
    }

    fclose(file);

    return NULL;
}