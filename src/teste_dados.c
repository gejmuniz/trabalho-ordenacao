/* Testes do gerador e do validador. Não faz parte do experimento:
   serve para confiar nessas peças antes de usá-las. */
#include <stdio.h>
#include <stdlib.h>
#include "dados.h"
#include "validacao.h"

static int falhas = 0;

static void checar(int condicao, const char *descricao) {
    printf("  [%s] %s\n", condicao ? " OK " : "FALHOU", descricao);
    if (!condicao) falhas++;
}

static void imprimir(const int *v, int n) {
    printf("  ");
    for (int i = 0; i < n; i++) printf("%d ", v[i]);
    printf("\n");
}

int main(void) {
    const int n = 20;
    const uint64_t semente = 123;

    /* 1) Inspeção visual de cada distribuição */
    printf("== Vetores gerados (n = %d, semente = %llu) ==\n", n, (unsigned long long)semente);
    for (int d = 0; d < NUM_DISTRIBUICOES; d++) {
        int *v = gerar_vetor((Distribuicao)d, n, semente);
        printf("%s:\n", NOMES_DISTRIBUICAO[d]);
        imprimir(v, n);
        free(v);
    }

    /* 2) Reprodutibilidade: mesma semente => mesmo vetor; semente diferente => vetor diferente */
    printf("\n== Reprodutibilidade ==\n");
    int *a = gerar_vetor(ALEATORIO, 1000, 7);
    int *b = gerar_vetor(ALEATORIO, 1000, 7);
    int *c = gerar_vetor(ALEATORIO, 1000, 8);
    int iguais_ab = 1, iguais_ac = 1;
    for (int i = 0; i < 1000; i++) {
        if (a[i] != b[i]) iguais_ab = 0;
        if (a[i] != c[i]) iguais_ac = 0;
    }
    checar(iguais_ab, "mesma semente gera o mesmo vetor");
    checar(!iguais_ac, "sementes diferentes geram vetores diferentes");

    /* 3) Aleatório sem repetição: ordenado, deve ser exatamente 1..n */
    printf("\n== Aleatorio e uma permutacao de 1..n ==\n");
    int nbig = 1000000;
    int *p = gerar_vetor(ALEATORIO, nbig, 2026);
    int *pr = gerar_referencia(p, nbig);
    int permutacao = 1;
    for (int i = 0; i < nbig; i++) if (pr[i] != i + 1) { permutacao = 0; break; }
    checar(permutacao, "n = 1.000.000 sem repeticao e sem valores faltando");
    checar(!esta_ordenado(p, nbig), "vetor aleatorio nao vem ordenado");
    free(p); free(pr);

    /* 4) O validador detecta cada tipo de erro */
    printf("\n== Validador ==\n");
    int *orig = gerar_vetor(POUCOS_DISTINTOS, 1000, 99);
    int *ref  = gerar_referencia(orig, 1000);

    int *certo = copiar_vetor(ref, 1000);
    checar(validar(certo, ref, 1000) == VALIDO, "resultado correto e aceito");

    int *desordenado = copiar_vetor(ref, 1000);
    int t = desordenado[0]; desordenado[0] = desordenado[999]; desordenado[999] = t;
    checar(validar(desordenado, ref, 1000) == ERRO_NAO_ORDENADO, "vetor fora de ordem e rejeitado");

    /* Erro traiçoeiro: ordenado, mas "perdeu" um elemento (trocado por uma cópia do vizinho) */
    int *perdeu = copiar_vetor(ref, 1000);
    int k = 1;
    while (k < 1000 && perdeu[k] == perdeu[k - 1]) k++;   /* acha uma fronteira entre valores */
    perdeu[k] = perdeu[k - 1];
    checar(esta_ordenado(perdeu, 1000), "(o vetor com elemento perdido continua ordenado)");
    checar(validar(perdeu, ref, 1000) == ERRO_ELEMENTOS, "vetor ordenado mas com elemento perdido e rejeitado");

    int *ig = gerar_vetor(IGUAIS, 1000, 0);
    int *igref = gerar_referencia(ig, 1000);
    checar(validar(ig, igref, 1000) == VALIDO, "vetor todo igual e considerado ordenado");

    free(a); free(b); free(c); free(orig); free(ref);
    free(certo); free(desordenado); free(perdeu); free(ig); free(igref);

    printf("\n%s (%d falha(s))\n", falhas == 0 ? "TODOS OS TESTES PASSARAM" : "HA FALHAS", falhas);
    return falhas != 0;
}
