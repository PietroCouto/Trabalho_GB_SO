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

    // TODO criar o mecanismo IPC

    int status;

    // Monitoring loop
    while (true)
    {
        pid_t returned_pid = waitpid(child_pid, &status, WNOHANG);

        if (returned_pid == -1)
            return_error("Erro ao executar o processo!");

        if (returned_pid > 0)
            break;

        /** TODO implementar a lógica de monitoramento
         *
         * Exibir informações gerais da execução, como total de palavras lidas, palavras
         * compactadas, palavras não encontradas no dicionário, tamanho original e compactado, e percentual
         * final de compressão
         *
         * acho que isso só vale para o modo de compressão...
         */
    }

    // TODO exibir mensagem de finalização (conteúdo original, arquivo compactado, arquivo descompactado)

    // Close the pipe
    close(pipe_fd[0]);
    
    return 0;
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

        // Redirect the children outputs
        dup2((*pipe_fd)[1], STDOUT_FILENO);
        dup2((*pipe_fd)[1], STDERR_FILENO);

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

        // Redirect the children outputs
        dup2((*pipe_fd)[1], STDOUT_FILENO);
        dup2((*pipe_fd)[1], STDERR_FILENO);

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
