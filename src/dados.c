#include <stdlib.h>
#include <string.h>
#include "dados.h"

const char *NOMES_DISTRIBUICAO[NUM_DISTRIBUICOES] = {
    "aleatorio", "crescente", "decrescente", "iguais", "poucos_distintos"
};

/* ---------- Gerador pseudoaleatório (splitmix64) ---------- */

static uint64_t estado_rng;

void rng_semear(uint64_t semente) {
    estado_rng = semente;
}

uint64_t rng_proximo(void) {
    uint64_t z = (estado_rng += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* ---------- Geração dos vetores ---------- */

int *gerar_vetor(Distribuicao d, int n, uint64_t semente) {
    int *v = malloc((size_t)n * sizeof(int));
    if (v == NULL) return NULL;

    /* Semeia aqui dentro: assim cada vetor depende só de (d, n, semente),
       e não da ordem em que os vetores foram gerados no programa. */
    rng_semear(semente);

    switch (d) {
    case ALEATORIO:
        /* Preenche 1..n e embaralha com Fisher-Yates:
           garante "sem repetição" e todas as permutações igualmente prováveis. */
        for (int i = 0; i < n; i++) v[i] = i + 1;
        for (int i = n - 1; i > 0; i--) {
            int j = (int)(rng_proximo() % (uint64_t)(i + 1));
            int tmp = v[i]; v[i] = v[j]; v[j] = tmp;
        }
        break;

    case CRESCENTE:
        for (int i = 0; i < n; i++) v[i] = i + 1;
        break;

    case DECRESCENTE:
        for (int i = 0; i < n; i++) v[i] = n - i;
        break;

    case IGUAIS:
        for (int i = 0; i < n; i++) v[i] = VALOR_IGUAL;
        break;

    case POUCOS_DISTINTOS:
        for (int i = 0; i < n; i++)
            v[i] = (int)(rng_proximo() % K_POUCOS_DISTINTOS);
        break;

    default:
        free(v);
        return NULL;
    }
    return v;
}

int *copiar_vetor(const int *v, int n) {
    int *c = malloc((size_t)n * sizeof(int));
    if (c != NULL) memcpy(c, v, (size_t)n * sizeof(int));
    return c;
}
