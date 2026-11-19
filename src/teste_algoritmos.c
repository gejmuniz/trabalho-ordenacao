/* Testa a CORREÇÃO dos algoritmos (não mede tempo).
   Compilar COM -DCONTAR para ver os contadores. */
#include <stdio.h>
#include <stdlib.h>
#include "dados.h"
#include "validacao.h"
#include "algoritmos.h"

typedef void (*FuncaoOrdenacao)(int *, int);

typedef struct {
    const char *nome;
    FuncaoOrdenacao funcao;
} Algoritmo;

/* Novos algoritmos entram nesta lista conforme forem implementados. */
static const Algoritmo ALGORITMOS[] = {
    { "bubble", bubble_sort },
    { "heap",   heap_sort   },
    { "merge",  merge_sort  },
};
static const int NUM_ALGORITMOS = sizeof(ALGORITMOS) / sizeof(ALGORITMOS[0]);

static int falhas = 0;

/* Roda um algoritmo numa cópia do vetor, valida e imprime os contadores. */
static void testar(const Algoritmo *a, Distribuicao d, int n) {
    int *original = gerar_vetor(d, n, 2026);
    int *referencia = gerar_referencia(original, n);
    int *v = copiar_vetor(original, n);

    zerar_contadores();
    a->funcao(v, n);
    int r = validar(v, referencia, n);

    printf("  %-8s %-17s n=%-6d %-6s comparacoes=%-11lld trocas=%lld\n",
           a->nome, NOMES_DISTRIBUICAO[d], n,
           r == VALIDO ? "OK" : "ERRO", comparacoes, trocas);
    if (r != VALIDO) falhas++;

    free(original); free(referencia); free(v);
}

int main(void) {
    /* Casos de borda: vetores vazios e minúsculos costumam revelar
       erros de índice (laços que começam/terminam no lugar errado). */
    printf("== Casos de borda ==\n");
    for (int i = 0; i < NUM_ALGORITMOS; i++)
        for (int n = 0; n <= 3; n++)
            testar(&ALGORITMOS[i], ALEATORIO, n);

    printf("\n== Todas as distribuicoes, n = 1000 ==\n");
    for (int i = 0; i < NUM_ALGORITMOS; i++)
        for (int d = 0; d < NUM_DISTRIBUICOES; d++)
            testar(&ALGORITMOS[i], (Distribuicao)d, 1000);

    printf("\n%s (%d falha(s))\n",
           falhas == 0 ? "TODOS OS TESTES PASSARAM" : "HA FALHAS", falhas);
    return falhas != 0;
}
