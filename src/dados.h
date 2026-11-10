#ifndef DADOS_H
#define DADOS_H

#include <stdint.h>

/* Os 5 tipos de entrada pedidos na especificação. */
typedef enum {
    ALEATORIO,          /* permutação de 1..n, sem repetição */
    CRESCENTE,          /* 1, 2, ..., n */
    DECRESCENTE,        /* n, n-1, ..., 1 */
    IGUAIS,             /* todos os elementos com o mesmo valor */
    POUCOS_DISTINTOS,   /* valores em [0, K_POUCOS_DISTINTOS) com muita repetição */
    NUM_DISTRIBUICOES
} Distribuicao;

/* Nomes usados no CSV e nos gráficos. */
extern const char *NOMES_DISTRIBUICAO[NUM_DISTRIBUICOES];

/* Quantos valores diferentes existem no cenário "poucos distintos". */
#define K_POUCOS_DISTINTOS 10

/* Valor usado no cenário "todos iguais". */
#define VALOR_IGUAL 42

/* Gerador pseudoaleatório próprio (splitmix64).
   Usamos um gerador próprio em vez de rand() porque:
   - no Windows, RAND_MAX = 32767, insuficiente para embaralhar 1.000.000 de posições;
   - rand() muda entre sistemas, e queremos que a mesma semente gere o mesmo vetor em qualquer PC. */
void     rng_semear(uint64_t semente);
uint64_t rng_proximo(void);

/* Aloca e devolve um vetor de tamanho n com a distribuição d.
   A semente só afeta ALEATORIO e POUCOS_DISTINTOS, mas é sempre registrada.
   Quem chama deve liberar o vetor com free(). Devolve NULL se faltar memória. */
int *gerar_vetor(Distribuicao d, int n, uint64_t semente);

/* Aloca e devolve uma cópia do vetor v (para cada algoritmo receber a mesma entrada). */
int *copiar_vetor(const int *v, int n);

#endif
