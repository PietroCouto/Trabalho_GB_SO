#include <stdio.h>
#include <string.h>

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
}

/**
 * Adds a single char to the end of a string
 * @param str Pointer to the string
 * @param character Char to be added
 */
void add_char(char *str, char character)
{
    // Get the current length of the string
    int length = strlen(str);

    // Add the new character at the end
    str[length] = character;

    // Add the null terminator
    str[length + 1] = '\0';
}

#endif