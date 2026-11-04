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