#include <stdio.h>
#include <unistd.h>
#include <string.h>

#ifndef UTILS_H
#define UTILS_H

//Struct with all types os stats
typedef struct{
    long read_bytes;
    long write_bytes;
    long compressed;
    long not_found;
    long restored;
} Stats;

//Send a copy of the stats by stdout (pipe to monitor)
void send_stats(const Stats *stats){
    write(STDOUT_FILENO, stats, sizeof(Stats));
}

/**
 * Prints an error message
 * @param message The message to be printed
 * @returns -1
 */
int return_error(char *message)
{
    fprintf(stderr, "%s\n", message);

    return -1;
}

/**
 * Adds a single char to the end of a string
 * @param s Pointer to the string
 * @param char Char to be added
 */
void add_char(char *s, char c)
{
    // Get the current length of the string
    int l = strlen(s);

    // Add the new character at the end
    s[l] = c;

    // Add the null terminator
    s[l + 1] = '\0';
}

#endif