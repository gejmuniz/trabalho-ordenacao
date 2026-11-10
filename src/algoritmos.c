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

/* ======================= HeapSort =======================
   Origem: versão clássica (slides da disciplina / Cormen, cap. 6),
   adaptada para índices a partir de 0 e sift-down iterativo.

   Ideia: o vetor é visto como uma árvore binária completa, guardada por níveis:
       pai de i   = (i - 1) / 2
       filhos de i = 2i + 1  e  2i + 2
   Um HEAP MÁXIMO é uma árvore em que todo pai é >= aos seus filhos,
   então o maior elemento está sempre na raiz v[0].

   Duas fases:
     1) CONSTRUÇÃO (Floyd, bottom-up): aplica sift-down do último pai até a raiz.
        Custo O(n) -- mais barato que inserir um a um, O(n log n).
     2) EXTRAÇÃO: troca a raiz (o maior) com o último elemento do heap,
        diminui o heap em 1 e conserta a raiz com sift-down. Repete n-1 vezes.
        Custo O(n log n).

   Características: NÃO estável (a troca da raiz com o fim "pula" elementos
   iguais), in-place, sem recursão (sift-down feito com laço).
   ========================================================= */

/* Faz o elemento v[i] "descer" até o lugar certo, num heap de tamanho n.
   Para quando o elemento é >= aos dois filhos ou quando chega numa folha. */
static void sift_down(int *v, int n, int i) {
    while (1) {
        int maior = i;
        int esq = 2 * i + 1;
        int dir = 2 * i + 2;

        if (esq < n) {
            CONTA_COMPARACAO();
            if (v[esq] > v[maior]) maior = esq;
        }
        if (dir < n) {
            CONTA_COMPARACAO();
            if (v[dir] > v[maior]) maior = dir;
        }

        if (maior == i) return;   /* pai já é >= filhos: heap consertado */

        int tmp = v[i]; v[i] = v[maior]; v[maior] = tmp;
        CONTA_TROCA();
        i = maior;                /* continua descendo a partir de onde o elemento foi */
    }
}

void heap_sort(int *v, int n) {
    /* Fase 1: construção. Folhas (índices >= n/2) já são heaps de 1 elemento,
       então começamos pelo último pai, n/2 - 1. */
    for (int i = n / 2 - 1; i >= 0; i--)
        sift_down(v, n, i);

    /* Fase 2: extração. Depois de cada troca, v[fim..n-1] está ordenado. */
    for (int fim = n - 1; fim > 0; fim--) {
        int tmp = v[0]; v[0] = v[fim]; v[fim] = tmp;   /* maior vai para o fim */
        CONTA_TROCA();
        sift_down(v, fim, 0);                          /* heap agora tem tamanho fim */
    }
}
