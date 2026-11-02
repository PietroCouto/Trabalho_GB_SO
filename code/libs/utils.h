#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

/**
 * Prints an error message
 * @param message The message to be printed
 * @returns -1
 */
int return_error(char *message)
{
    fprintf(stderr, "%s\n", message);

    return -1;
};

#endif