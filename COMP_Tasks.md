# Épico: Corrigir `code/compress.c`

Tasks em ordem de execução: as primeiras bloqueiam as seguintes. Linhas se referem ao `compress.c` atual.

## Resumo

| ID | Título | Tipo | Prioridade | Status |
|----|--------|------|------------|--------|
| COMP-3 | Tokenização incompatível com o formato do `mini_zip` | Bug | Alta | [x] validado (`entrada.txt` igual ao `mz -c`) |
| COMP-4 | Palavra pode ultrapassar o buffer de acumulação | Bug | Média | [ ] |
| COMP-7 | Falhas de abertura de arquivo não avisam ninguém | Bug | Alta | [~] parcial |
| COMP-8 | Fila sem limite de tamanho (sem backpressure) | Melhoria | Média | [~] só o refactor |
| COMP-9 | Vazamentos de memória | Bug | Baixa | [~] parcial |
| COMP-10 | Estatísticas e IPC com o Monitor | Feature | Alta | [ ] |
| COMP-11 | Palavras de 1-2 letras com maiúscula ("O") ficam sem símbolo | Melhoria | Baixa | [ ] |
| COMP-12 | Escape para símbolos que aparecem no texto original (`:` do `entrada.txt`) | Bug | Alta | [ ] |

Legenda: `[x]` feito · `[~]` parcialmente feito (ver "Estado" em cada task) · `[ ]` não iniciado.
Status conferido contra o código em `395d0b7`.

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

**Estado:** `[x]` implementado; falta só validar.
- [x] Item 1 e 2: `strtok` removido; `compress_line` percorre caractere a caractere e grava o separador intacto (`compress.c:279-327`).
- [x] Item 4: sem `strtok`, o problema de thread-safety deixou de existir.
- [x] Item 3: o `tolower` foi movido para uma cópia usada só na busca no dicionário (`flush_word`). A palavra é acumulada com a caixa original, e as que começam com maiúscula e casam com o dicionário saem como `SHIFT_CHAR` (0x0E, em `dictionary.h`) + símbolo. Palavras de 1-2 letras ou com maiúsculas no meio ficam literais. O cast `(unsigned char)` foi dispensado de propósito: os dois arquivos de teste são ASCII puro.
- [ ] O **descompactador ainda não trata o `SHIFT_CHAR`**: ao ver o byte 0x0E ele deve ligar um flag `shift_next` (mantido entre blocos) e passar a primeira letra da próxima palavra traduzida para maiúscula.
- Consequência: a saída do `entrada2.txt` deixa de ser idêntica à do `mz -c` (o `entrada.txt` continua idêntico). O critério de aceite passa a ser o round-trip `descompactar(compactar(x)) == x`.
- [ ] Ainda não validado byte a byte contra `mz -c` (nenhum teste/diff no repositório).

---

## COMP-4 · Palavra pode ultrapassar o buffer de acumulação
**Tipo:** Bug (segurança de memória) · **Prioridade:** Média · **Local:** vai surgir ao resolver COMP-3

**Problema:** O `mini_zip` usa `char w[256]` e nunca verifica `wl`. Uma palavra com 256+ letras estoura o buffer. Também não há proteção contra o `data[1024]` do nó.

**Solução:** Verificar `wl < sizeof(w) - 1` antes de acumular. Se a palavra for maior, descarregá-la como "não encontrada" e recomeçar. Fazer o mesmo para o buffer de saída: o pior caso é a saída do tamanho da entrada (símbolo ≤ palavra), então um buffer do mesmo tamanho do de entrada basta.

**Estado:** `[ ]` não feito. `word[256]` em `compress.c:283` é escrito sem checar `word_length` (linha 291). Os `add_char`/`strcat` no `data[1024]` também não têm limite (hoje seguro só porque `fgets` lê no máximo 1023 bytes, mas uma linha maior é partida em dois nós e pode cortar uma palavra ao meio).

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

**Estado:** `[~]` parcial.
- [x] `argc` validado no início do `main` (`compress.c:70-74`).
- [x] Erro no `reader`: agora chama `finish_queue` (lock → flag → broadcast → unlock), então o compressor não dorme para sempre (`compress.c:159-164`).
- [ ] Erro no `writer` (`compress.c:246-247`): ainda só dá `return NULL`; o compressor continua enchendo a fila sem consumidor.
- [ ] Nenhum código de erro compartilhado: `main` retorna `0` sempre (`compress.c:108`), então o Monitor (`main.c`) não sabe se falhou. Também falta checar o `status` do `waitpid` lá.
- [ ] Os arquivos ainda são abertos dentro das threads, não no `main`.

---

## COMP-8 · Fila sem limite de tamanho (sem backpressure)
**Tipo:** Melhoria técnica · **Prioridade:** Média · **Local:** `compress.c:28-34` e `reader`

**Problema:** O `reader` enfileira linhas sem parar, porque não há limite. Em arquivos grandes, ele lê tudo para a memória antes de o compressor fazer qualquer coisa. O enunciado pede comunicação do tipo produtor/consumidor com "gerenciamento de recursos", e na apresentação isso será questionado.

**Solução:** Adicionar `count` e `capacity` à `SharedQueue`, mais uma segunda variável de condição `not_full`. O produtor espera enquanto a fila estiver cheia, e o consumidor sinaliza `not_full` depois de retirar um item. Extrair `queue_push`/`queue_pop` como funções, pois o código de lock/wait/signal está duplicado nas 3 threads.

**Estado:** `[~]` só o refactor.
- [x] Extração feita em `code/libs/queue.h` (`insert_element`, `get_element`, `finish_queue`, `start_queue`).
- [ ] `SharedQueue` não tem `count`/`capacity` nem `not_full`; `insert_element` nunca bloqueia, então o `reader` ainda lê o arquivo inteiro para a memória.

---

## COMP-9 · Vazamentos de memória
**Tipo:** Bug · **Prioridade:** Baixa · **Local:** `compress.c:216` e `writer`

**Problema:** Além do `malloc` do símbolo (COMP-2), se o `writer` não abrir o arquivo (`return NULL` antes do loop), os nós restantes nunca são liberados. As filas também nunca são destruídas (`pthread_mutex_destroy`/`cond_destroy`) no `main`.

**Solução:** Liberar tudo no final e destruir mutexes e conds no `main`, depois dos `join`s.

**Estado:** `[~]` parcial.
- [x] COMP-2 deixou de existir (sem `malloc` de símbolo); `free(element)` no compressor e no writer.
- [x] `mutex_destroy`/`cond_destroy`/`free` das duas filas no `main` (`compress.c:99-106`).
- [ ] Se o `writer` falha no `fopen`, os nós da fila continuam sem `free`. Falta também esvaziar qualquer nó restante antes de destruir a fila.
- [ ] `calloc` do `reader`/`compressor` não é checado contra `NULL`.

---

## COMP-11 · Palavras de 1-2 letras com maiúscula ("O") ficam sem símbolo
**Tipo:** Melhoria · **Prioridade:** Baixa · **Local:** `flush_word` e `libs/dictionary.h` · **Depende de:** dicionário ampliado

**Problema:** `SHIFT_CHAR` + símbolo ocupa 2 bytes, então `flush_word` deixa literal toda palavra com maiúscula de até 2 letras (`length <= 2`). No `entrada2.txt` isso afeta os 4 "O": ficam como texto, enquanto o `mz -c` os compacta. O tamanho final é o mesmo (447 bytes), mas essas palavras não contam como "compactadas" para o Monitor.

**Solução (só vale a pena junto com o dicionário ampliado):**
1. Em `dictionary.h`, adicionar a entrada `"O"` em `WORD` **e** um símbolo novo na mesma posição de `SYM`. As duas listas têm que ter o mesmo tamanho, porque `DICTIONARY_SIZE` vem de `SYM`. O símbolo não pode ser nenhum dos atuais, nem `0x0E` (SHIFT), nem `0x0F` (reservado ao escape), nem `\n`, `\r` ou tab. Um byte como `0x80` serve, pois os testes são ASCII puro.
2. Em `flush_word`, logo depois de `word[*word_length] = '\0';`, tentar o casamento exato antes de qualquer coisa:
```c
int exact = word_to_sym(word);
if (exact >= 0)
{
    add_char(output, (char)exact);
    return;
}
```
Só adicionar a palavra no dicionário **não basta**: sem o passo 2, a regra `length <= 2` ainda manda o "O" para texto literal.

**Descompactador:** não muda, desde que use o mesmo `dictionary.h`.

**Observações:** não reduz bytes (1 byte vira 1 byte). Palavras de 3+ letras ("Este", "Assim") ficam melhor com o `SHIFT_CHAR` e a versão minúscula no dicionário. Alternativa aceitável: deixar como está e explicar na apresentação.

---

## COMP-12 · Escape para símbolos que aparecem no texto original
**Tipo:** Bug · **Prioridade:** Alta · **Local:** `compress_line` e `decompress.c`

**Problema:** o `entrada.txt` tem 2 caracteres `:` ("threads: o processo"), e `:` é um dos símbolos do dicionário. O descompactador troca o `:` por uma palavra, e o round-trip fica diferente do original (testado com o `mz`: "threads**:** o processo" volta como "threads**arquivo** o processo"). Qualquer dígito ou pontuação que seja símbolo tem o mesmo risco.

**Solução:** um segundo byte de controle de escape (sugestão: `0x0F`, ASCII SI). Na compactação, todo byte do texto que coincida com um símbolo do dicionário (ou com os bytes de controle) sai como `ESC` + byte. Na descompactação, ao ver `ESC`, copiar o próximo byte sem traduzir. Mesma técnica do `SHIFT_CHAR`, e o estado também precisa atravessar blocos.

---

## COMP-10 · Estatísticas e IPC com o Monitor
**Tipo:** Feature · **Prioridade:** Alta · **Depende de:** COMP-3

**Problema:** O compressor não conta palavras nem bytes, e não tem como informar o Monitor. Sem isso, o requisito do enunciado não é atendido.

**Solução:** Definir um header comum (`shared.h`) com a struct `Stats` (total de palavras, compactadas, não encontradas, tamanho original, tamanho compactado, flag `finished`). Criar a memória compartilhada com `shm_open` + `mmap`, com mutex `PTHREAD_PROCESS_SHARED`. Cada thread atualiza o que é seu:
- `reader`: tamanho original;
- `compressor`: contagem de palavras;
- `writer`: tamanho final.

Fica para depois de arrumar o compressor.

**Estado:** `[ ]` não iniciado. Não existe `shared.h`; o compressor não conta nada; `main.c` só tem TODOs no lugar do IPC e do monitoramento. O `decompress.c` já inclui um `shared.h` e usa `stats_open()`/`stats_t` que ninguém definiu, então o header precisa servir aos dois (os campos usados lá são `compressed_size`, `decompressed_size`, `words_count`, `words_size`, `finished`).

---
