#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef DICTIONARY_H
#define DICTIONARY_H

static const unsigned char SYM[] =
    {'@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '=', '{', '}', '[', ']', '<', '>', '/', '\\', '|', '~', '`', ':', ';', '?', '!', '\"', '\'', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static const char *WORD[] =
    {"de", "que", "para", "com", "nao", "por", "em", "uma", "quando", "muito", "o", "as",
     "ao", "mais", "como", "se", "e", "foi", "tem", "tambem", "programa", "processos", "threads", "arquivo",
     "arquivos", "texto", "dicionario", "palavras", "compressao", "exemplo", "objetivo", "leitura",
     "gravar", "relatorio", "entrada", "saida", "aluno", "testes", "atividade"};

static const int N = sizeof(SYM) / sizeof(SYM[0]);

static int word_to_sym(const char *w)
{ // palavra → símbolo
    for (int i = 0; i < N; i++)
        if (!strcmp(WORD[i], w))
            return SYM[i];
    return -1;
}

static const char *sym_to_word(int c)
{ // símbolo → palavra
    for (int i = 0; i < N; i++)
        if (SYM[i] == (unsigned char)c)
            return WORD[i];
    return NULL;
}

#endif