#include "algoritmos.h"

long long comparacoes = 0;
long long trocas = 0;

void zerar_contadores(void) {
    comparacoes = 0;
    trocas = 0;
}

/* ======================= BubbleSort =======================
   Origem: versão clássica (slides da disciplina / Cormen), com duas melhorias:
     1) flag "houve_troca": se uma passada inteira não troca nada,
        o vetor já está ordenado e o algoritmo para.
        -> é isso que dá o melhor caso O(n) no vetor crescente.
     2) limite que encolhe: depois da passada k, os k maiores elementos
        já estão no fim, então a passada seguinte vai uma posição a menos.
   Características: estável (só troca se v[j] > v[j+1], nunca iguais),
   in-place, iterativo (sem recursão).
   ========================================================== */
void bubble_sort(int *v, int n) {
    for (int fim = n - 1; fim > 0; fim--) {
        int houve_troca = 0;

        for (int j = 0; j < fim; j++) {
            CONTA_COMPARACAO();
            if (v[j] > v[j + 1]) {          /* ">" e não ">=": mantém a estabilidade */
                int tmp = v[j];
                v[j] = v[j + 1];
                v[j + 1] = tmp;
                CONTA_TROCA();
                houve_troca = 1;
            }
        }

        if (!houve_troca) break;            /* passada sem trocas: já está ordenado */
    }
}
