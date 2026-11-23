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

/* ======================= QuickSort =======================
   Origem: Cormen, cap. 7 (Lomuto) e problema 7-1 (Hoare); mediana de três
   conforme Sedgewick. Duas versões, para comparar o efeito da IMPLEMENTAÇÃO.

   Ideia (dividir e conquistar): escolhe um PIVÔ e PARTICIONA o trecho:
   menores (ou iguais) de um lado, maiores (ou iguais) do outro.
   Depois ordena cada lado recursivamente. Não precisa de "merge":
   depois da partição, cada lado já está no lugar certo em relação ao outro.

   O custo depende de quão EQUILIBRADAS são as partições:
     - meio a meio  -> log2(n) níveis  -> O(n log n)
     - 0 e n-1      -> n níveis        -> O(n^2) e recursão com profundidade n

   Características: NÃO estável (a partição troca elementos distantes),
   in-place (mas usa a pilha da recursão), recursivo.
   ========================================================= */

static void trocar(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
    CONTA_TROCA();
}

/* ---------- Versão INGÊNUA: pivô = último elemento, partição de Lomuto ----------
   Existe para DEMONSTRAR o pior caso:
     - vetor crescente: o pivô (último) é o MAIOR -> partições de n-1 e 0;
     - vetor decrescente: o pivô é o MENOR -> mesma coisa;
     - todos iguais: "a[j] <= pivo" é sempre verdadeiro -> todos vão para
       a esquerda -> partições de n-1 e 0.
   Em todos esses casos: O(n^2) e profundidade de recursão ~n,
   o que pode ESTOURAR A PILHA com n grande (resultado esperado no estresse).
   Detalhe desta implementação: não troca um elemento com ele mesmo (i != j),
   então só conta trocas que realmente movem elementos. */

static int particiona_lomuto(int *a, int esq, int dir) {
    int pivo = a[dir];
    int i = esq - 1;                 /* fim da região dos "<= pivo" */

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
        trocar(&a[i + 1], &a[dir]);  /* põe o pivô entre as duas regiões */
    }

    return i + 1;                    /* posição final do pivô */
}

static void quick_ingenuo_rec(int *a, int esq, int dir) {
    if (esq < dir) {
        int p = particiona_lomuto(a, esq, dir);
        quick_ingenuo_rec(a, esq, p - 1);    /* o pivô já está no lugar certo */
        quick_ingenuo_rec(a, p + 1, dir);
    }
}

void quick_sort_ingenuo(int *a, int n) {
    if (a != NULL && n > 1) {
        quick_ingenuo_rec(a, 0, n - 1);
    }
}

/* ---------- Versão MELHORADA: mediana de três + partição de Hoare ----------
   Três decisões, cada uma corrigindo um problema da versão ingênua:

   1) MEDIANA DE TRÊS: o pivô é a mediana entre v[ini], v[meio] e v[fim].
      Em vetores crescentes/decrescentes, o elemento do meio é a mediana
      exata -> partições perfeitamente equilibradas.

   2) PARTIÇÃO DE HOARE: dois índices andam um em direção ao outro e
      ambos PARAM em elementos IGUAIS ao pivô, trocando-os. Assim os iguais
      se dividem entre os dois lados -> com todos iguais, partições meio a meio.
      (Na de Lomuto, todos os iguais iam para o mesmo lado.)

   3) RECURSÃO SÓ NO LADO MENOR: o lado maior é tratado pelo laço while
      (eliminação da recursão de cauda). Como o lado menor tem no máximo
      metade dos elementos, a profundidade da pilha fica <= log2(n),
      mesmo no pior caso -> nunca estoura a pilha. */

/* Ordena v[ini], v[meio], v[fim] entre si; a mediana fica em v[meio]. */
static int mediana_de_tres(int *v, int ini, int fim) {
    int meio = ini + (fim - ini) / 2;
    CONTA_COMPARACAO(); if (v[meio] < v[ini])  trocar(&v[meio], &v[ini]);
    CONTA_COMPARACAO(); if (v[fim]  < v[ini])  trocar(&v[fim],  &v[ini]);
    CONTA_COMPARACAO(); if (v[fim]  < v[meio]) trocar(&v[fim],  &v[meio]);
    return v[meio];
}

/* Devolve j tal que v[ini..j] <= pivo <= v[j+1..fim]
   (o pivô NÃO fica necessariamente na posição final, diferente de Lomuto). */
static int particiona_hoare(int *v, int ini, int fim) {
    int pivo = mediana_de_tres(v, ini, fim);
    int i = ini - 1;
    int j = fim + 1;

    while (1) {
        do { i++; CONTA_COMPARACAO(); } while (v[i] < pivo);   /* para em >= pivo */
        do { j--; CONTA_COMPARACAO(); } while (v[j] > pivo);   /* para em <= pivo */
        if (i >= j) return j;                                  /* índices se cruzaram */
        trocar(&v[i], &v[j]);
    }
}

static void quick_rec(int *v, int ini, int fim) {
    while (ini < fim) {
        int j = particiona_hoare(v, ini, fim);

        if (j - ini < fim - j) {          /* lado esquerdo é o menor */
            quick_rec(v, ini, j);         /*   recursão no menor      */
            ini = j + 1;                  /*   laço continua no maior */
        } else {                          /* lado direito é o menor   */
            quick_rec(v, j + 1, fim);
            fim = j;
        }
    }
}

void quick_sort(int *v, int n) {
    if (v != NULL && n > 1) {
        quick_rec(v, 0, n - 1);
    }
}
