#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#ifndef STATS_H
#define STATS_H

// Struct with all types of stats
typedef struct
{
    long read_bytes;
    long write_bytes;
    long compressed;
    long not_found;
    long restored;
    pthread_mutex_t mutex;
} Stats;

/**
 * Send a copy of the stats by stdout (pipe to monitor)
 * @param stats The stats structure to be sent
 */
void send_stats(Stats *stats)
{

    pthread_mutex_lock(&stats->mutex);
    write(STDOUT_FILENO, stats, sizeof(Stats));
    pthread_mutex_unlock(&stats->mutex);
}

/**
 * Displays the compressor mode status
 * @param stats The stats structure containing the data
 */
void show_compressed_stats(Stats *stats)
{
    printf("Palavras totais: %ld\n", stats->compressed + stats->not_found);
    printf("Palavras compactadas: %ld\n", stats->compressed);
    printf("Palavras fora do dicionário: %ld\n", stats->not_found);
    printf("Tamanho original: %ld bytes\n", stats->read_bytes);
    printf("Tamanho compactado: %ld bytes\n", stats->write_bytes);

    if (stats->read_bytes > 0)
        printf("Compressão: %.2f%%\n", 100.0 * (stats->read_bytes - stats->write_bytes) / stats->read_bytes);
}

/**
 * Displays the decompressor mode status
 * @param stats The stats structure containing the data
 */
void show_decompressed_stats(Stats *stats)
{
    printf("Tamanho compactado: %ld bytes\n", stats->read_bytes);
    printf("Tamanho descompactado: %ld bytes\n", stats->write_bytes);
    printf("Palavras restauradas: %ld\n", stats->restored);
}

/**
 * Updates the read_bytes stats
 * @param stats The stats structure to be updated
 * @param line The string to count the bytes from
 */
void update_read_data(Stats *stats, char *line)
{
    pthread_mutex_lock(&stats->mutex);
    stats->read_bytes += strlen(line);
    pthread_mutex_unlock(&stats->mutex);
}

/**
 * Updates the compressed and not found words
 * @param stats The stats structure to be updated
 * @param compressed The total compressed words
 * @param not_found The total not found words
 */
void update_compressed_data(Stats *stats, int compressed, int not_found)
{
    pthread_mutex_lock(&stats->mutex);
    stats->compressed += compressed;
    stats->not_found += not_found;
    pthread_mutex_unlock(&stats->mutex);
}

/**
 * Updates the write bytes
 * @param stats The stats structure to be updated
 * @param data The string to count the bytes from
 */
void update_write_data(Stats *stats, char *data)
{
    pthread_mutex_lock(&stats->mutex);
    stats->write_bytes += strlen(data);
    pthread_mutex_unlock(&stats->mutex);

    send_stats(stats);
}

#endif