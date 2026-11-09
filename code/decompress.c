/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

/**
 * Libraries
 */
#include <stdio.h>
#include <stdlib.h>

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
    SharedQueue *decompressor_queue;
} DecompressorStructure;

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
void startup_structures(ReaderStructure *reader_structure, DecompressorStructure *compressor_structure, WriterStructure *writer_structure, char *input_path, char *output_path);
static void *writer(void *arg);
static void *decompressor(void *arg);
static void *reader(void *arg);
void decompress_line(struct node *element, struct node *decompressed_element);

/**
 * Main function
 * @param argc Argument count
 * @param argv Arguments
 * @returns -1 for error, 0 for success
 */
int main(int argc, char *argv[])
{
    if (argc < 3)
        return return_error("Argumentos insuficientes para o descompactador!");

    if (argc > 3)
        return return_error("Argumentos em excesso para o descompactador!");

    // Create the structures for the threads
    ReaderStructure reader_structure;
    DecompressorStructure decompressor_structure;
    WriterStructure writer_structure;

    // Start the structures and queues
    startup_structures(&reader_structure, &decompressor_structure, &writer_structure, argv[1], argv[2]);

    // Create the thread IDs
    pthread_t reader_thread;
    pthread_t decompressor_thread;
    pthread_t writer_thread;

    pthread_create(&reader_thread, NULL, reader, &reader_structure);
    pthread_create(&decompressor_thread, NULL, decompressor, &decompressor_structure);
    pthread_create(&writer_thread, NULL, writer, &writer_structure);

    pthread_join(reader_thread, NULL);
    pthread_join(decompressor_thread, NULL);
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
void startup_structures(ReaderStructure *reader_structure, DecompressorStructure *compressor_structure, WriterStructure *writer_structure, char *input_path, char *output_path)
{
    // Create the buffers
    // Allocate the buffers on the heap
    SharedQueue *reader_queue = malloc(sizeof(SharedQueue));
    SharedQueue *decompressor_queue = malloc(sizeof(SharedQueue));

    // Initialize queues
    start_queue(reader_queue);
    start_queue(decompressor_queue);

    // Assign the heap pointers to the structures
    reader_structure->queue = reader_queue;

    compressor_structure->reader_queue = reader_queue;
    compressor_structure->decompressor_queue = decompressor_queue;

    writer_structure->queue = decompressor_queue;

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
    FILE *file = fopen(reader_structure->filepath, "rb");

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

static void *decompressor(void *arg)
{
    // Get the compressor structure
    DecompressorStructure *decompressor_structure = (DecompressorStructure *)arg;

    // Extract the queues
    SharedQueue *reader_queue = decompressor_structure->reader_queue;
    SharedQueue *decompressor_queue = decompressor_structure->decompressor_queue;

    while (true)
    {
        // Try to get the next element in the reader queue
        struct node *element = get_element(reader_queue);

        // Check if the reader queue has ended
        if (element == NULL)
            break;

        // Create the node to hold the compressed data
        struct node *decompressed_element = calloc(1, sizeof(struct node));

        // Compress the element line
        decompress_line(element, decompressed_element);

        // Insert the compressed line into the queue
        insert_element(decompressor_queue, decompressed_element);

        // Free the element memory
        free(element);
    }

    // End the compressor queue
    finish_queue(decompressor_queue);

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
    {
        finish_queue(queue);
        return NULL;
    }

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
 * Decompresses a line and writes it to the decompressed element
 * @param element The element containing the line to be decompressed
 * @param decompressed_element The element to hold the decompressed data
 */
void decompress_line(struct node *element, struct node *decompressed_element)
{
    int character;

    // Clear the destination string
    decompressed_element->data[0] = '\0';

    // Iterate the compressed line
    for (int i = 0; (character = element->data[i]) != '\0'; i++)
    {
        // Try to translate the symbol
        char *word = sym_to_word(character);

        // If it was a dictionary hit
        if (word)
        {
            strcat(decompressed_element->data, word);
            continue;
        }

        // If it was a miss
        add_char(decompressed_element->data, character);
    }
}