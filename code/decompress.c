/**
 * @authors Pietro do Couto Freitas, Luiz Sergio Chisté Fogaça e Vitor Martins de Souza
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
 
#include "dictionary.h"
#include "shared.h"

#define IN_CHUNK 4096 //Bytes lidos do arquivo compactado 
#define QUEUE_SIZE 8 //Tamanho da fila de blocos descompactados


typedef struct {
    char *data;
    size_t len;
} chunk_t;

typedef struct {
    chunk_t *SLOTS[QUEUE_SLOTS];
    int head, tail, count;
    int closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} queue_t;

static void queue_init(queue_t *q){
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

static void queue_destroy(queue_t *q){
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
} 

static void queue_push(queue_t *q, chunk_t *chunk){
    pthread_mutex_lock(&q->mutex);
    while(q->count == QUEUE_SLOTS){
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
;
    q->SLOTS[q->tail] = chunk;
    q->tail = (q->tail + 1) % QUEUE_SLOTS;
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

static int queue_pop(queue_t *q, chunk_t *c){
    pthread_mutex_lock(&q->mutex);
    while(q->count == 0 && !q->closed){
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    if(q->count == 0){
        pthread_mutex_unlock(&q->mutex);
        return 0; 
    }

    *c= q->slots[q->head];
    q->head = (q->head + 1) % QUEUE_SLOTS;
    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return 1; 
}



static void queue_close(queue_t *q){
    pthread_mutex_lock(&q->mutex);
    q->closed = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}


typedef struct {
    FILE *in;
    FILE *out;
    queue_t *q_in;
    queue_t *q_out;
    stats_t *stats;
    const char *table[256];
    size_t max_word_len;
    int error;
} ctx_t;

#define STAT_ADD(ctx, field, n)                                  
    do{                                                            
        if ((ctx)->stats)                                        
            atomic_fetch_add(&(ctx)->stats->field, (unsigned long)(n)); 
    } while (0)


static void build_table(ctx_t *ctx){
    memset(ctx->table, 0, sizeof(ctx->table));
    ctx->max_word_len = 1;

    for(int i=0; i < DICT_SIZE; i++){
        ctx->table[SYM[i]] = WORD[i];
        if(strlen(WORD[i]) > ctx->max_word_len)
            ctx->max_word_len = strlen(WORD[i]);
    }
} 

static void *reader_thread(void *arg){
    ctx_t *ctx = arg; 

    while(1){
        char *buf = malloc(IN_CHUNK);
        if(!buf){
            ctx->error = 1;
            break;
        }

        size_t n = fread(buf, 1, IN_CHUNK, ctx->in);
        if(n == 0){
            free(buf);
            if (FERROR(ctx->in))
                ctx->error = 1;
            break;
        }  
        STAT_ADD(ctx, compressed_size, n);
        queue_push(&ctx->q_in, (chunk_t){buf, n});
    }

    queue_close(ctx->q_in);
    return NULL;
}

static void *decompressor_thread(void *arg){
    ctx_t *ctx = arg; 
    chunk_t in;
    int in_word = 0;

    while(queue_pop(&ctx->q_in, &in)){
        char *out = malloc(in.len * ctx->max_word_len + 1);
        if(!out){
            ctx->error = 1;
            free(in.data);
            continue;
        }

        size_t out = 0;
        for(size_t i=0; i < in.len; i++){
            unsigned char c = (unsigned char)in.data[i];
            const char *word = ctx->table[c];
            if(word){
                size_t word_len = strlen(word);
                memcpy(out + out, word, word_len);
                out += word_len;
            } else{
                out[out++] = (char)c;

                if (isalnum(c) || c == '_') {
                    if (!in_word) {
                        STAT_ADD(ctx, words_count, 1);
                        STAT_ADD(ctx, words_size, 1);
                    }
                    in_word = 1;
                } else {
                    in_word = 0;
                }
            }
            free(in.data);
            queue_push(&ctx->q_out, (chunk_t){out, out});
        }
    }
    queue_close(&ctx->q_out);
    return NULL;
}


static void *writer_thread(void *arg){
    ctx_t *ctx = arg; 
    chunk_t out;

    while(queue_pop(&ctx->q_out, &c)){
      if (fwrite(c.data, 1, c.len, ctx->out) != c.len){
            ctx->error = 1;
    
        }
        STAT_ADD(ctx, decompressed_size, out.len);
        free(out.data);
    }
    return NULL;
}



/**
 * Main function
 * @param argc Argument count
 * @param argv Arguments
 * @returns -1 for error, 0 for success
 */
int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Uso: %s arquivo.mzp [saida.txt]\n", argv[0]);
        return -1;
    }

    char out_name[512];
    if (argc == 3) {
        snprintf(out_name, sizeof(out_name), "%s", argv[2]);
    } else {
        build_output_name(argv[1], sizeof(out_name), out_name);
    }

    static ctx_t ctx;
    ctx.in = fopen(argv[1], "rb");
    if (!ctx.in) {
        perror("Erro ao abrir arquivo de entrada");
        return -1;
    }

    ctx.out = fopen(out_name, "wb");
    if (!ctx.out) {
        perror("Erro ao abrir arquivo de saída");
        return -1;
    }

    build_table(&ctx);
    queue_init(&ctx.q_in);
    queue_init(&ctx.q_out);
    ctx.stats = stats_open();

    pthread_t t_read, t_dec, t_write;
    pthread_create(&t_read, NULL, reader_thread, &ctx);
    pthread_create(&t_dec, NULL, decompressor_thread, &ctx);
    pthread_create(&t_write, NULL, writer_thread, &ctx);

    pthread_join(t_read, NULL);
    pthread_join(t_dec, NULL);
    pthread_join(t_write, NULL);

    fclose(ctx.in);
    fclose(ctx.out);

    queue_destroy(&ctx.q_in);
    queue_destroy(&ctx.q_out);

    if (ctx.stats) {
        atomic_store(&ctx.stats->finished, 1);
        munmap(ctx.stats, sizeof(stats_t));
    }

    if(ctx.error) {
        fprintf(stderr, "Erro durante a descompactação\n");
        return -1;
    }

    return 0;
}