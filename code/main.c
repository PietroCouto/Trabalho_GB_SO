/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "libs/utils.h"

/**
 * Prototypes
 */
int start_compress(char *input_path, char *output_path, int (*pipe_fd)[2]);
int start_decompress(char *input_path, char *output_path, int (*pipe_fd)[2]);
void print_file(char *title, char *path);

/**
 * Main function
 * @param argc Argument count
 * @param argv Arguments
 * @returns -1 for error, 0 for success
 */
int main(int argc, char *argv[])
{
    // Check if enough arguments were provided
    if (argc < 4)
        return return_error("Argumentos insuficientes!");

    // Check if to many arguments were provided
    if (argc > 4)
        return return_error("Argumentos em excesso!");

    // Check if a valid option was provided
    if (strcmp(argv[1], "compress") != 0 && strcmp(argv[1], "decompress") != 0)
        return return_error("Opção inválida!");

    pid_t child_pid = -1;

    int pipe_fd[2];

    if (pipe(pipe_fd) < 0)
        exit(1);

    // Check if compress mode was selected
    if (strcmp(argv[1], "compress") == 0)
    {
        // Try to start the compressing process
        child_pid = start_compress(argv[2], argv[3], &pipe_fd);

        // Check if the startup was successful
        if (child_pid == -1)
            return return_error("Erro ao inicializar o compactador!");

        printf("Compactador inicializado! PID %i\n", child_pid);
    }

    // Check if decompress mode was selected
    if (strcmp(argv[1], "decompress") == 0)
    {
        // Try to start the decompressing process
        child_pid = start_decompress(argv[2], argv[3], &pipe_fd);

        // Check if the startup was successful
        if (child_pid == -1)
            return return_error("Erro ao inicializar o descompactador!");

        printf("Descompactador inicializado! PID %i\n", child_pid);
    }

    close(pipe_fd[1]);

    Stats stats = {0};
    Stats incoming;
    size_t received = 0;
    ssize_t bytes;

    // Read the stats sent by the child until it closes the pipe
    while ((bytes = read(pipe_fd[0], (char *)&incoming + received, sizeof(Stats) - received)) > 0)
    {
        received += bytes;

        // Check if the struct is still incomplete
        if (received < sizeof(Stats))
            continue;

        // Each struct already has the totals, so just replace the old one
        received = 0;
        stats = incoming;

        printf("\rLidos: %ld bytes | Escritos: %ld bytes", stats.read_bytes, stats.write_bytes);
        fflush(stdout);
    }

    printf("\n");

    // Close the pipe
    close(pipe_fd[0]);

    int status;

    // Wait for the child to end
    if (waitpid(child_pid, &status, 0) == -1)
        return return_error("Erro ao aguardar o processo!");

    // Check if the child ended with an error
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return return_error("O processo terminou com erro!");

    // Show the final report
    if (strcmp(argv[1], "compress") == 0)
    {
        printf("Palavras totais: %ld\n", stats.compressed + stats.not_found);
        printf("Palavras compactadas: %ld\n", stats.compressed);
        printf("Palavras fora do dicionário: %ld\n", stats.not_found);
        printf("Tamanho original: %ld bytes\n", stats.read_bytes);
        printf("Tamanho compactado: %ld bytes\n", stats.write_bytes);

        if (stats.read_bytes > 0)
            printf("Compressão: %.2f%%\n", 100.0 * (stats.read_bytes - stats.write_bytes) / stats.read_bytes);

        print_file("Conteúdo original", argv[2]);
        print_file("Arquivo compactado", argv[3]);
    }
    else
    {
        printf("Tamanho compactado: %ld bytes\n", stats.read_bytes);
        printf("Tamanho descompactado: %ld bytes\n", stats.write_bytes);
        printf("Palavras restauradas: %ld\n", stats.restored);

        print_file("Arquivo compactado", argv[2]);
        print_file("Arquivo descompactado", argv[3]);
    }

    return 0;
}

/**
 * Prints the content of a file
 * Control characters are shown as <0xNN> to not mess up the terminal
 * @param title The title to be shown before the content
 * @param path The file to be printed
 */
void print_file(char *title, char *path)
{
    printf("\n--- %s (%s) ---\n", title, path);

    // Try to open the file
    FILE *file = fopen(path, "rb");

    // Check for failure
    if (file == NULL)
    {
        printf("Não foi possível abrir o arquivo!\n");
        return;
    }

    int character;

    while ((character = fgetc(file)) != EOF)
    {
        // Keep the line breaks and tabs, replace the other control characters
        if (character == '\n' || character == '\r' || character == '\t' || (character >= 32 && character < 127))
            putchar(character);
        else
            printf("<0x%02X>", character);
    }

    printf("\n");

    fclose(file);
}

/**
 * Creates the compressing process
 * @param input_path The file to be decompressed
 * @param output_path The file to be written to
 * @param pipe_fd The pipe file descriptors for communication
 * @returns -1 for error, PID for success
 */
int start_compress(char *input_path, char *output_path, int (*pipe_fd)[2])
{
    pid_t pid = fork();

    // Check if the fork was successful
    if (pid == -1)
        return -1;

    // Try to replace the child code
    if (pid == 0)
    { 
        // Close the children pipe stdin
        close((*pipe_fd)[0]);

        // Redirect only the children stdout, errors still go to the terminal
        dup2((*pipe_fd)[1], STDOUT_FILENO);

        // Close the children stdout
        close((*pipe_fd)[1]);

        char *argv[] = {"./code/compiled/compress", (char *)input_path, (char *)output_path, NULL};

        char *envp[] = {NULL};

        // Replace the child process
        execve("./code/compiled/compress", argv, envp);

        // Kill the child in case of execve failure
        _exit(EXIT_FAILURE);
    }

    return pid;
};

/**
 * Creates the decompressing process
 * @param input_path The file to be decompressed
 * @param output_path The file to be written to
 * @param pipe_fd The pipe file descriptors for communication
 * @returns -1 for error, PID for success
 */
int start_decompress(char *input_path, char *output_path, int (*pipe_fd)[2])
{
    pid_t pid = fork();

    // Check if the fork was successful
    if (pid == -1)
        return -1;

    // Try to replace the child code
    if (pid == 0)
    {
        // Close the children pipe stdin
        close((*pipe_fd)[0]);

        // Redirect only the children stdout, errors still go to the terminal
        dup2((*pipe_fd)[1], STDOUT_FILENO);

        // Close the children stdout
        close((*pipe_fd)[1]);

        char *argv[] = {"./code/compiled/decompress", (char *)input_path, (char *)output_path, NULL};

        char *envp[] = {NULL};

        // Replace the child process
        execve("./code/compiled/decompress", argv, envp);

        // Kill the child in case of execve failure
        _exit(EXIT_FAILURE);
    }

    return pid;
};
