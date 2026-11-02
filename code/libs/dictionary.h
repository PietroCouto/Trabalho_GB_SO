#include <stdio.h>
#include <string.h>

#ifndef DICTIONARY_H
#define DICTIONARY_H

/**
 * The available symbols
 */
static const unsigned char SYM[] =
    {'@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '=', '{', '}', '[', ']', '<', '>', '/', '\\', '|', '~', '`', ':', ';', '?', '!', '\"', '\'', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

/**
 * The dictionary
 */
static const char *WORD[] =
    {"de", "que", "para", "com", "nao", "por", "em", "uma", "quando", "muito", "o", "as",
     "ao", "mais", "como", "se", "e", "foi", "tem", "tambem", "programa", "processos", "threads", "arquivo",
     "arquivos", "texto", "dicionario", "palavras", "compressao", "exemplo", "objetivo", "leitura",
     "gravar", "relatorio", "entrada", "saida", "aluno", "testes", "atividade"};

/**
 * The dictionary size
 */
static const int DICTIONARY_SIZE = sizeof(SYM) / sizeof(SYM[0]);

/**
 * Converts words to symbols
 * @param word Word to be translated
 * @returns -1 for error, SYM for success
 */
static int word_to_sym(const char *word)
{
    for (int i = 0; i < DICTIONARY_SIZE; i++)
        if (!strcmp(WORD[i], word))
            return SYM[i];

    return -1;
}

/**
 * Converts symbols to words
 * @param symbol Integer representing the symbol to translate
 * @returns NULL for no match, WORD for match
 */
static const char *sym_to_word(int symbol)
{
    for (int i = 0; i < DICTIONARY_SIZE; i++)
        if (SYM[i] == (unsigned char)symbol)
            return WORD[i];

    return NULL;
}

#endif