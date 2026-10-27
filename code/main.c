/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

/**
 * Prototypes
 */

int return_error(char *message);
int start_compress(char *filename);
int start_decompress(char *filename);

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
    if (!strcmp(argv[1], "compress") && !strcmp(argv[1], "decompress"))
        return return_error("Opção inválida!");

    pid_t child_pid = -1;

    // Check if compress mode was selected
    if (strcmp(argv[1], "compress") == 0)
    {
        // Try to start the compressing process
        child_pid = start_compress(argv[2]);

        // Check if the startup was successful
        if (child_pid == -1)
            return return_error("Erro ao inicializar o compactador!");

        printf("Compactador inicializado! PID %i\n", child_pid);
    }

    // Check if decompress mode was selected
    if (strcmp(argv[1], "decompress") == 0)
    {
        // Try to start the decompressing process
        child_pid = start_decompress(argv[2]);

        // Check if the startup was successful
        if (child_pid == -1)
            return return_error("Erro ao inicializar o descompactador!");

        printf("Descompactador inicializado! PID %i\n", child_pid);
    }

    // TODO criar o mecanismo IPC

    int status;

    // Monitoring loop
    while (true)
    {
        pid_t returned_pid = waitpid(child_pid, &status, 0);

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

    return 0;
}

/**
 * Prints an error message
 * @param message The message to be printed
 * @returns -1
 */
int return_error(char *message)
{
    printf("%s\n", message);

    return -1;
};

/**
 * Creates the compressing process
 * @param filename The name of the file to be compressed
 * @returns -1 for error, PID for success
 */
int start_compress(char *filename)
{
    pid_t pid = fork();

    // Check if the fork was successful
    if (pid == -1)
        return -1;

    // Try to replace the child code
    if (pid == 0)
    {
        char *argv[] = {"./compress", (char *)filename, NULL};

        char *envp[] = {NULL};

        // Replace the child process
        execve("./compress", argv, envp);

        // Kill the child in case of execve failure
        _exit(EXIT_FAILURE);
    }

    return pid;
};

/**
 * Creates the decompressing process
 * @param filename The name of the file to be decompressed
 * @returns -1 for error, PID for success
 */
int start_decompress(char *filename)
{
    pid_t pid = fork();

    // Check if the fork was successful
    if (pid == -1)
        return -1;

    // Try to replace the child code
    if (pid == 0)
    {
        char *argv[] = {"./decompress", (char *)filename, NULL};

        char *envp[] = {NULL};

        // Replace the child process
        execve("./decompress", argv, envp);

        // Kill the child in case of execve failure
        _exit(EXIT_FAILURE);
    }

    return pid;
};
