#include <stdio.h>
#include <string.h>

#ifndef DICTIONARY_H
#define DICTIONARY_H

#define SHIFT_CHAR 0x0E // ControlChar: the word next to it start with uppercase

/**
 * The available symbols
 */
static const unsigned char SYM[] = {
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2D,
    0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xE3,
    0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x42, 0x43, 0x44, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
    0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x62,
    0x63, 0x64, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C,
    0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
    0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
    0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0,
    0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC,
    0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8,
    0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4,
    0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
    0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC,
    0xDD, 0xDE, 0xDF, 0xE1, 0xE2};

/**
 * The dictionary
 */
static const char *WORD[] =
    {"a", "c", "e", "o", "A", "E", "O", "de", "em", "as", "ao", "se", "no", "um", "ou",
     "os", "da", "do", "com", "nao", "por", "uma", "que", "foi", "tem", "ler", "faz",
     "uso", "ver", "pai", "ser", "mas", "ele", "ate", "mais", "como", "para", "este",
     "pode", "foco", "aqui", "taxa", "modo", "isso", "util", "seja", "deve", "duas",
     "caso", "fato", "cada", "fica", "reduz", "muito", "texto", "saida", "aluno",
     "ideia", "ainda", "assim", "igual", "ajuda", "facil", "vezes", "comum", "mesmo",
     "final", "exibe", "sejam", "grava", "foque", "linha", "longo", "filho", "menor",
     "sobre", "possui", "quando", "gravar", "testes", "tambem", "criado", "direto",
     "logica", "maxima", "ganhar", "comuns", "muitas", "linhas", "varias", "termos",
     "muitos", "conter", "metodo", "curtas", "textos", "porque", "apenas", "normal",
     "frases", "ganhos", "contem", "tantas", "varios", "inclui", "juntos", "mostra",
     "repete", "efeito", "criados", "utiliza", "entrada", "threads", "arquivo",
     "exemplo", "leitura", "avaliar", "simples", "mostrar", "formato", "falamos",
     "pequeno", "repetem", "solucao", "palavra", "maiores", "simbolo", "espacos",
     "tamanho", "comprova", "programa", "arquivos", "palavras", "objetivo", "material",
     "possivel", "processo", "aparecem", "original", "tecnicos", "continua", "observar",
     "entender", "bastante", "escrever", "pensamos", "encontra", "encontre", "concluir",
     "trabalha", "eficiente", "demonstra", "relatorio", "processos", "atividade",
     "preservar", "repetidas", "diferenca", "portugues", "trabalhar", "estimular", "pontuacao",
     "contextos", "preparado", "trabalham", "comprimir", "estruturas", "paragrafos",
     "escolhemos", "diferentes", "repeticoes", "frequentes", "exatamente", "academicos",
     "organizado", "combinacoes", "semelhantes", "ocorrencias", "compactacao", "aprendizado",
     "compressao", "dicionario", "estatisticas", "descompactado", "correspondente"};

/**
 * The dictionary size
 */
static const int DICTIONARY_SIZE = sizeof(SYM) / sizeof(SYM[0]);

/**
 * Converts words to symbols
 * @param word Word to be translated
 * @returns -1 for no match, >= 0 for success
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
 * @returns NULL for no match, WORD for match (points to the dictionary, do not free)
 */
static const char *sym_to_word(int symbol)
{
    for (int i = 0; i < DICTIONARY_SIZE; i++)
        if (SYM[i] == (unsigned char)symbol)
            return WORD[i];

    return NULL;
}

#endif