/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

/**
 * Libraries
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/**
 * Handmade Libraries
 */
#include "libs/dictionary.h"
#include "libs/utils.h"
#include "libs/queue.h"

/**
 * ReaderStructure
 * Contains the reader thread parameter structure
 */
typedef struct
{
    SharedQueue *queue;
    char *filepath;
} ReaderStructure;

/**
 * CompressorStructure
 * Contains the compressor thread parameter structure
 */
typedef struct
{
    SharedQueue *reader_queue;
    SharedQueue *compressor_queue;
} CompressorStructure;

/**
 * WriterStructure
 * Contains the writer thread parameter structure
 */
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
void compress_line(struct node *element, struct node *compressed_element);

/**
 * Compresses a file
 * @param argc Argument count
 * @param argv Arguments input_filepath and output_filepath
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

    // Clear the queues
    pthread_mutex_destroy(&reader_structure.queue->mutex);
    pthread_cond_destroy(&reader_structure.queue->cond);
    free(reader_structure.queue);

    pthread_mutex_destroy(&writer_structure.queue->mutex);
    pthread_cond_destroy(&writer_structure.queue->cond);
    free(writer_structure.queue);

    return 0;
}

/**
 * Creates the queues and assembles the structures
 * @param reader_structure Pointer to the reader structure
 * @param compressor_structure Pointer to the compressor structure
 * @param writer_structure Pointer to the writer structure
 * @param input_path The input file path
 * @param output_path The output file path
 */
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

    // Finishes the reader queue
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
    // Get the compressor structure
    CompressorStructure *compressor_structure = (CompressorStructure *)arg;

    // Extract the queues
    SharedQueue *reader_queue = compressor_structure->reader_queue;
    SharedQueue *compressor_queue = compressor_structure->compressor_queue;

    while (true)
    {
        // Try to get the next element in the reader queue
        struct node *element = get_element(reader_queue);

        // Check if the reader queue has ended
        if (element == NULL)
            break;

        // Create the node to hold the compressed data
        struct node *compressed_element = calloc(1, sizeof(struct node));

        // Compress the element line
        compress_line(element, compressed_element);

        // Insert the compressed line into the queue
        insert_element(compressor_queue, compressed_element);

        // Free the element memory
        free(element);
    }

    // End the compressor queue
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
    SharedQueue *queue = writer_structure->queue;

    // Try to open the output file
    FILE *file = fopen(writer_structure->filepath, "w");

    // Check for failure to open the output file
    if (file == NULL)
        return NULL;

    while (true)
    {
        // Try to get the next element from the queue
        struct node *element = get_element(queue);

        // Check if the queue has ended
        if (element == NULL)
            break;

        // Write the element to the output file
        fprintf(file, "%s", element->data);

        // Flush the file to force fprintf to write
        fflush(file);

        // Free the element memory
        free(element);
    }

    // Close the output file
    fclose(file);

    return NULL;
}

/**
 * Compresses a string node
 * @param element The node containing the string to be compressed
 * @param compresssed_element The node to hold the compressed string
 */
void compress_line(struct node *element, struct node *compressed_element)
{
    int character;
    int word_length = 0;
    char word[256];

    // Loop through the entire line
    for(int i = 0; (character = element->data[i]) != '\0'; i++)
    {
        // Check if the character is part of a word
        if (isalnum(character) || character == '_')
        {
            word[word_length++] = character;
            continue;
        }

        // Check if it is a word
        if (word_length)
        {
            word[word_length] = 0;
            word_length = 0;

            // Try to compress the word
            int symbol = word_to_sym(word);

            // Check if the word matches the dictionary
            if (symbol >= 0)
                add_char(compressed_element->data, (char)symbol);

            // If it does not, simply add it
            else
                strcat(compressed_element->data, word);
        }

        // If we have a standalone character, just add it
        add_char(compressed_element->data, (char)character);
    }

    if (word_length)
    {
        word[word_length] = 0;
        int symbol = word_to_sym(word);

        if (symbol >= 0)
            add_char(compressed_element->data, (char)symbol);
        else
            strcat(compressed_element->data, word);
    }
}