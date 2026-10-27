/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Souza
 */
#include <unistd.h>

/**
 * Prototypes
 */

int return_error(char message[]);
int start_compress(char filename[]);
int start_decompress(char filename[]);

/**
 * Main function
 * @param argc Argument count
 * @param argv Arguments
 * @returns -1 for error, 0 for success
 */
int main(int argc, char *argv[])
{
    // Check if enough arguments were provided
    if (argc < 3)
        return return_error("Argumentos insuficientes!");

    // Check if to many arguments were provided
    if (argc > 3)
        return return_error("Argumentos em excesso!");

    // Check if a valid option was provided
    if (argv[1] != "compress" && argv[1] != "decompress")
        return return_error("Opção inválida!");

    pid_t child_pid = -1;

    if (argv[1] == "compress")
    {
        child_pid = start_compress(argv[2]);

        if (child_pid == -1)
            return return_error("Erro ao inicializar o compactador!");
    }

    if (argv[1] == "decompress")
    {
        child_pid = start_decompress(argv[2]);

        if (child_pid == -1)
            return return_error("Erro ao inicializar o descompactador!");
    }

    return 0;
}

/**
 * Print a message
 * @param message The message to be printed
 * @returns -1
 */
int return_error(char message[])
{
    printf(message);

    return -1;
};

/**
 *
 */
int start_compress(char filename[])
{
    pid_t pid = fork();

    if (pid == -1)
        return -1;

    execve("./compress.c", filename, NULL);

    return pid;
};

/**
 *
 */
int start_decompress(char filename[])
{
    pid_t pid = fork();

    if (pid == -1)
        return -1;

    execve("./decompress.c", filename, NULL);

    return pid;
};
