#include <stdlib.h>
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

/* ======================= MergeSort =======================
   Origem: versão clássica top-down (slides da disciplina / Cormen, cap. 2).

   Ideia (dividir e conquistar):
     1) DIVIDIR: parte o vetor ao meio, recursivamente, até pedaços de 1 elemento
        (um pedaço de 1 elemento já está ordenado).
     2) INTERCALAR (merge): junta duas metades já ordenadas numa só,
        sempre pegando o menor entre os "primeiros da fila" de cada metade.

   Custo: log2(n) níveis de divisão; em cada nível, os merges movem n elementos
   no total -> O(n log n) em QUALQUER distribuição (a divisão nunca depende
   dos valores, sempre corta no meio).

   Decisões:
     - Vetor auxiliar alocado UMA VEZ em merge_sort() e reaproveitado em todas
       as chamadas. Alocar dentro da recursão faria n-1 mallocs e estaríamos
       medindo o malloc, não o algoritmo.
     - "<=" no merge: entre iguais, o da metade ESQUERDA vem primeiro
       -> mantém a ordem original dos iguais -> ESTÁVEL.
     - SEM a otimização "se v[meio] <= v[meio+1], pular o merge":
       ela tornaria o vetor ordenado O(n) e esconderia a característica
       teórica principal do MergeSort (tempo igual em qualquer distribuição).
     - Contadores: o MergeSort não faz trocas; o contador "trocas" registra
       cada CÓPIA de elemento (para o vetor auxiliar e de volta).

   Características: estável, NÃO in-place (auxiliar de n elementos),
   recursivo (profundidade da recursão ~ log2(n): ~20 para n = 1.000.000).
   ========================================================= */

/* Intercala v[ini..meio] e v[meio+1..fim], ambos já ordenados. */
static void intercalar(int *v, int *aux, int ini, int meio, int fim) {
    /* Copia o trecho para o auxiliar; o resultado é escrito de volta em v. */
    for (int k = ini; k <= fim; k++) {
        aux[k] = v[k];
        CONTA_TROCA();
    }

    int i = ini;        /* próximo da metade esquerda (em aux) */
    int j = meio + 1;   /* próximo da metade direita  (em aux) */
    int k = ini;        /* próxima posição a preencher em v   */

    /* Enquanto as duas metades têm elementos: pega o menor. */
    while (i <= meio && j <= fim) {
        CONTA_COMPARACAO();
        if (aux[i] <= aux[j])           /* "<=": empate favorece a esquerda (estável) */
            v[k++] = aux[i++];
        else
            v[k++] = aux[j++];
        CONTA_TROCA();
    }

    /* Uma metade acabou: o que sobrou da outra já está ordenado,
       basta copiar (sem comparações). */
    while (i <= meio) { v[k++] = aux[i++]; CONTA_TROCA(); }
    while (j <= fim)  { v[k++] = aux[j++]; CONTA_TROCA(); }
}

static void merge_sort_rec(int *v, int *aux, int ini, int fim) {
    if (ini >= fim) return;                 /* 0 ou 1 elemento: já ordenado */

    int meio = ini + (fim - ini) / 2;       /* evita overflow de (ini + fim) */
    merge_sort_rec(v, aux, ini, meio);
    merge_sort_rec(v, aux, meio + 1, fim);
    intercalar(v, aux, ini, meio, fim);
}

void merge_sort(int *v, int n) {
    if (n < 2) return;

    int *aux = malloc((size_t)n * sizeof(int));
    if (aux == NULL) return;   /* sem memória: o vetor fica desordenado
                                  e o validador acusa a falha */

    merge_sort_rec(v, aux, 0, n - 1);
    free(aux);
}
static void trocar(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
    CONTA_TROCA();
}

static int particionar(int *a, int esq, int dir) {
    int pivo = a[dir];
    int i = esq - 1;

    for (int j = esq; j < dir; j++) {
        CONTA_COMPARACAO();
        if (a[j] <= pivo) {
            i++;
            if (i != j) {
                trocar(&a[i], &a[j]);
            }
        }
    }

    if (i + 1 != dir) {
        trocar(&a[i + 1], &a[dir]);
    }

    return i + 1;
}

static void quick_sort_recursivo(int *a, int esq, int dir) {
    if (esq < dir) {
        int p = particionar(a, esq, dir);
        quick_sort_recursivo(a, esq, p - 1);
        quick_sort_recursivo(a, p + 1, dir);
    }
}

void quick_sort(int *a, int n) {
    if (a != NULL && n > 1) {
        quick_sort_recursivo(a, 0, n - 1);
    }
}