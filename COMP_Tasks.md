# Épico: Corrigir `code/compress.c`

Tasks em ordem de execução: as primeiras bloqueiam as seguintes. Linhas se referem ao `compress.c` atual.

## Resumo

| ID | Título | Tipo | Prioridade | Status |
|----|--------|------|------------|--------|
| COMP-3 | Tokenização incompatível com o formato do `mini_zip` | Bug | Alta | [ ] |
| COMP-4 | Palavra pode ultrapassar o buffer de acumulação | Bug | Média | [ ] |
| COMP-7 | Falhas de abertura de arquivo não avisam ninguém | Bug | Alta | [ ] |
| COMP-8 | Fila sem limite de tamanho (sem backpressure) | Melhoria | Média | [ ] |
| COMP-9 | Vazamentos de memória | Bug | Baixa | [ ] |
| COMP-10 | Estatísticas e IPC com o Monitor | Feature | Alta | [ ] |

### Ordem recomendada

1. COMP-3 e COMP-4: núcleo da correção. Na prática viram uma única função "comprimir uma linha".
2. COMP-7, COMP-9 e COMP-8.
3. COMP-10: depende do grupo.

---

## COMP-3 · Tokenização incompatível com o formato do `mini_zip`
**Tipo:** Bug · **Prioridade:** Alta · **Local:** `compress.c:203, 209-229`

**Problema:** Quatro defeitos num só lugar:
1. `strtok(" ")` separa só por espaço. Em `"entender.\n"` e `"assim,"` a pontuação e o `\n` ficam grudados na palavra, que então nunca casa com o dicionário.
2. `strtok` colapsa espaços repetidos e perde o separador original. O código re-adiciona sempre 1 espaço (`strcat(" ")`), então tabs, `\n`, `\r` (o `entrada.txt` é CRLF) e espaços duplos viram outra coisa. O descompactado não vai ser idêntico.
3. Não existe `tolower`. O `mini_zip` compara em minúsculo, e o `entrada2.txt` tem maiúsculas.
4. O `strtok` não é thread-safe e guarda estado global (use `strtok_r` se for mantê-lo). Aqui só uma thread o usa, mas é uma armadilha.

**Solução:** Substituir o `strtok` por um laço caractere a caractere, igual ao do `mini_zip.c`:
- se o caractere é alfanumérico ou `_`, acumula em `word[]` (já em minúsculo);
- senão, fecha a palavra: busca no dicionário e grava o símbolo ou a palavra; depois grava o separador **intacto**;
- no final da linha, fecha a palavra que ficou pendente.

Isso também elimina o problema do item 2, porque o separador original nunca é perdido.

**Critério de aceite:** o resultado do `compress` é byte a byte igual ao do `mz -c` (a única exceção possível é o tratamento de maiúsculas, que dá para combinar com o grupo).

---

## COMP-4 · Palavra pode ultrapassar o buffer de acumulação
**Tipo:** Bug (segurança de memória) · **Prioridade:** Média · **Local:** vai surgir ao resolver COMP-3

**Problema:** O `mini_zip` usa `char w[256]` e nunca verifica `wl`. Uma palavra com 256+ letras estoura o buffer. Também não há proteção contra o `data[1024]` do nó.

**Solução:** Verificar `wl < sizeof(w) - 1` antes de acumular. Se a palavra for maior, descarregá-la como "não encontrada" e recomeçar. Fazer o mesmo para o buffer de saída: o pior caso é a saída do tamanho da entrada (símbolo ≤ palavra), então um buffer do mesmo tamanho do de entrada basta.

---

## COMP-7 · Falhas de abertura de arquivo não avisam ninguém
**Tipo:** Bug · **Prioridade:** Alta · **Local:** `compress.c:131-135, 262-263`; `main` (linha 67)

**Problema:**
- O `main` não valida `argc`. Rodar sem argumentos faz `argv[1]` ser `NULL` e dá segfault.
- Se o `reader` não consegue abrir o arquivo, só marca `is_finished = 1` **sem lock** e sem `broadcast`. O compressor pode ficar dormindo para sempre.
- Se o `writer` falha no `fopen`, retorna e o `compressor` continua enchendo a fila sem ninguém consumir.
- O programa retorna `0` mesmo com erro, então o `main.c` (Monitor) acha que deu certo.

**Solução:**
- Validar `argc == 3` no início do `main`.
- No erro do reader, fazer a mesma sequência do caso normal (lock → `is_finished = 1` → broadcast → unlock).
- Guardar um código de erro compartilhado e fazer `main` devolver `1` se algum fopen falhar.
- Abrir os dois arquivos no `main` antes de criar as threads, para que o erro apareça cedo.

---

## COMP-8 · Fila sem limite de tamanho (sem backpressure)
**Tipo:** Melhoria técnica · **Prioridade:** Média · **Local:** `compress.c:28-34` e `reader`

**Problema:** O `reader` enfileira linhas sem parar, porque não há limite. Em arquivos grandes, ele lê tudo para a memória antes de o compressor fazer qualquer coisa. O enunciado pede comunicação do tipo produtor/consumidor com "gerenciamento de recursos", e na apresentação isso será questionado.

**Solução:** Adicionar `count` e `capacity` à `SharedQueue`, mais uma segunda variável de condição `not_full`. O produtor espera enquanto a fila estiver cheia, e o consumidor sinaliza `not_full` depois de retirar um item. Extrair `queue_push`/`queue_pop` como funções, pois o código de lock/wait/signal está duplicado nas 3 threads.

---

## COMP-9 · Vazamentos de memória
**Tipo:** Bug · **Prioridade:** Baixa · **Local:** `compress.c:216` e `writer`

**Problema:** Além do `malloc` do símbolo (COMP-2), se o `writer` não abrir o arquivo (`return NULL` antes do loop), os nós restantes nunca são liberados. As filas também nunca são destruídas (`pthread_mutex_destroy`/`cond_destroy`) no `main`.

**Solução:** Liberar tudo no final e destruir mutexes e conds no `main`, depois dos `join`s.

---

## COMP-10 · Estatísticas e IPC com o Monitor
**Tipo:** Feature · **Prioridade:** Alta · **Depende de:** COMP-3

**Problema:** O compressor não conta palavras nem bytes, e não tem como informar o Monitor. Sem isso, o requisito do enunciado não é atendido.

**Solução:** Definir um header comum (`shared.h`) com a struct `Stats` (total de palavras, compactadas, não encontradas, tamanho original, tamanho compactado, flag `finished`). Criar a memória compartilhada com `shm_open` + `mmap`, com mutex `PTHREAD_PROCESS_SHARED`. Cada thread atualiza o que é seu:
- `reader`: tamanho original;
- `compressor`: contagem de palavras;
- `writer`: tamanho final.

Fica para depois de arrumar o compressor.

---
