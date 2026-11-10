#include <stdlib.h>
#include <string.h>
#include "validacao.h"

static int comparar_int(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);   /* evita overflow de "x - y" */
}

int *gerar_referencia(const int *original, int n) {
    int *ref = malloc((size_t)n * sizeof(int));
    if (ref == NULL) return NULL;
    memcpy(ref, original, (size_t)n * sizeof(int));
    qsort(ref, (size_t)n, sizeof(int), comparar_int);
    return ref;
}

int esta_ordenado(const int *v, int n) {
    for (int i = 1; i < n; i++)
        if (v[i - 1] > v[i]) return 0;
    return 1;
}

int validar(const int *resultado, const int *referencia, int n) {
    /* Primeiro testa a ordem, para o código de erro dizer QUAL foi o problema. */
    if (!esta_ordenado(resultado, n)) return ERRO_NAO_ORDENADO;

    /* Se está ordenado e é idêntico à referência, então contém exatamente
       os mesmos elementos da entrada (com as mesmas repetições). */
    if (memcmp(resultado, referencia, (size_t)n * sizeof(int)) != 0)
        return ERRO_ELEMENTOS;

    return VALIDO;
}
