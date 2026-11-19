/* Programa de medição: roda UM cenário (algoritmo, distribuição, n)
   com várias repetições e grava uma linha no CSV por execução.

   Uso:
     ./medir  <algoritmo> <distribuicao> <n> <repeticoes> <semente> <arquivo.csv>
     ./contar <algoritmo> <distribuicao> <n> <semente> <arquivo.csv>

   Compilação (dentro de src/):
     tempo:    gcc -O2 -Wall -Wextra -std=c11          -o medir  main.c algoritmos.c dados.c validacao.c
     contagem: gcc -O2 -Wall -Wextra -std=c11 -DCONTAR -o contar main.c algoritmos.c dados.c validacao.c

   Código de saída: 0 = todas as execuções válidas; 1 = erro de uso/memória;
                    2 = alguma execução falhou na validação.
   Status "timeout", "crash" e "nao_executado" NÃO são gravados aqui:
   quem os grava é o script que roda a matriz (o processo morreu ou nem rodou). */

/* Necessário com -std=c11 para o clock_gettime aparecer fora do Windows. */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dados.h"
#include "validacao.h"
#include "algoritmos.h"

/* ---------------- Relógio de alta precisão ---------------- */
#ifdef _WIN32
  #include <windows.h>
  static double agora_s(void) {
      static LARGE_INTEGER freq = {0};
      LARGE_INTEGER t;
      if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
      QueryPerformanceCounter(&t);
      return (double)t.QuadPart / (double)freq.QuadPart;
  }
#else
  #include <time.h>
  static double agora_s(void) {
      struct timespec t;
      clock_gettime(CLOCK_MONOTONIC, &t);
      return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
  }
#endif

/* ---------------- Medição em lote ----------------
   Com n pequeno, uma ordenação leva menos de 1 microssegundo, perto da
   resolução do relógio (~0,1 us no Windows). Para medir com precisão,
   cada "repetição" ordena um LOTE de K cópias do mesmo vetor, todas
   preparadas ANTES de ligar o relógio, e o tempo gravado é total / K.
   K é escolhido para que o lote tenha ~ELEMENTOS_POR_LOTE elementos:
     n = 100 -> K = 1000;  n = 1.000 -> K = 100;  n = 10.000 -> K = 10;
     n >= 100.000 -> K = 1 (medição direta). */
#define ELEMENTOS_POR_LOTE 100000

static int tamanho_lote(int n) {
#ifdef CONTAR
    (void)n;
    return 1;               /* contagem não depende do relógio */
#else
    if (n <= 0 || n >= ELEMENTOS_POR_LOTE) return 1;
    return ELEMENTOS_POR_LOTE / n;
#endif
}

/* ---------------- Tabela de algoritmos ----------------
   Para acrescentar um algoritmo: uma linha aqui (o nome é o que vai no CSV). */
typedef void (*FuncaoOrdenacao)(int *, int);
typedef struct { const char *nome; FuncaoOrdenacao funcao; } Algoritmo;

static const Algoritmo ALGORITMOS[] = {
    { "bubble", bubble_sort },
    { "heap",   heap_sort   },
    { "merge",  merge_sort  },
    /* Descomentar quando o QuickSort estiver em algoritmos.c: */
    /* { "quick",         quick_sort         }, */
    /* { "quick_ingenuo", quick_sort_ingenuo }, */
};
static const int NUM_ALGORITMOS = sizeof(ALGORITMOS) / sizeof(ALGORITMOS[0]);

static const Algoritmo *buscar_algoritmo(const char *nome) {
    for (int i = 0; i < NUM_ALGORITMOS; i++)
        if (strcmp(ALGORITMOS[i].nome, nome) == 0) return &ALGORITMOS[i];
    return NULL;
}

static int buscar_distribuicao(const char *nome) {
    for (int d = 0; d < NUM_DISTRIBUICOES; d++)
        if (strcmp(NOMES_DISTRIBUICAO[d], nome) == 0) return d;
    return -1;
}

/* Abre o CSV para acrescentar linhas; escreve o cabeçalho se o arquivo estiver vazio. */
static FILE *abrir_csv(const char *caminho, const char *cabecalho) {
    FILE *f = fopen(caminho, "a");
    if (f == NULL) return NULL;
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) fprintf(f, "%s\n", cabecalho);
    return f;
}

static void uso(const char *prog) {
#ifdef CONTAR
    fprintf(stderr, "Uso: %s <algoritmo> <distribuicao> <n> <semente> <arquivo.csv>\n", prog);
#else
    fprintf(stderr, "Uso: %s <algoritmo> <distribuicao> <n> <repeticoes> <semente> <arquivo.csv>\n", prog);
#endif
    fprintf(stderr, "Algoritmos:");
    for (int i = 0; i < NUM_ALGORITMOS; i++) fprintf(stderr, " %s", ALGORITMOS[i].nome);
    fprintf(stderr, "\nDistribuicoes:");
    for (int d = 0; d < NUM_DISTRIBUICOES; d++) fprintf(stderr, " %s", NOMES_DISTRIBUICAO[d]);
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
#ifdef CONTAR
    const int ARGS = 6;
#else
    const int ARGS = 7;
#endif
    if (argc != ARGS) { uso(argv[0]); return 1; }

    const Algoritmo *alg = buscar_algoritmo(argv[1]);
    int dist = buscar_distribuicao(argv[2]);
    int n = atoi(argv[3]);
#ifdef CONTAR
    int repeticoes = 1;     /* contagem é determinística: uma execução basta */
    unsigned long long semente = strtoull(argv[4], NULL, 10);
    const char *caminho = argv[5];
#else
    int repeticoes = atoi(argv[4]);
    unsigned long long semente = strtoull(argv[5], NULL, 10);
    const char *caminho = argv[6];
#endif

    if (alg == NULL || dist < 0 || n < 0 || repeticoes < 1) { uso(argv[0]); return 1; }

    /* Preparação (fora da medição): entrada, referência e K vetores de trabalho. */
    int lote = tamanho_lote(n);
    int *original   = gerar_vetor((Distribuicao)dist, n, semente);
    int *referencia = original ? gerar_referencia(original, n) : NULL;
    int *v          = malloc((size_t)(n > 0 ? n : 1) * (size_t)lote * sizeof(int));
    if (original == NULL || referencia == NULL || v == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente para n = %d\n", n);
        return 1;
    }

#ifdef CONTAR
    FILE *csv = abrir_csv(caminho, "algoritmo,distribuicao,n,semente,comparacoes,trocas,status");
#else
    FILE *csv = abrir_csv(caminho, "algoritmo,distribuicao,n,repeticao,semente,tempo_s,status");
#endif
    if (csv == NULL) { fprintf(stderr, "Erro: nao foi possivel abrir %s\n", caminho); return 1; }

    int houve_erro = 0;

    for (int r = 1; r <= repeticoes; r++) {
        /* K cópias da mesma entrada, preparadas antes de ligar o relógio. */
        for (int k = 0; k < lote; k++)
            memcpy(v + (size_t)k * n, original, (size_t)n * sizeof(int));
        zerar_contadores();

        double t0 = agora_s();
        for (int k = 0; k < lote; k++)
            alg->funcao(v + (size_t)k * n, n);           /* SÓ isto é medido */
        double t1 = agora_s();
        double tempo = (t1 - t0) / lote;                 /* tempo de UMA ordenação */

        /* Valida todas as cópias do lote. */
        int valido = 1;
        for (int k = 0; k < lote; k++)
            if (validar(v + (size_t)k * n, referencia, n) != VALIDO) valido = 0;
        if (!valido) houve_erro = 1;
        const char *status = valido ? "ok" : "erro_validacao";

#ifdef CONTAR
        (void)tempo;
        fprintf(csv, "%s,%s,%d,%llu,%lld,%lld,%s\n",
                alg->nome, NOMES_DISTRIBUICAO[dist], n, semente,
                comparacoes, trocas, status);
#else
        if (valido)
            fprintf(csv, "%s,%s,%d,%d,%llu,%.9f,%s\n",
                    alg->nome, NOMES_DISTRIBUICAO[dist], n, r, semente, tempo, status);
        else   /* tempo de uma execução incorreta não vale: fica vazio */
            fprintf(csv, "%s,%s,%d,%d,%llu,,%s\n",
                    alg->nome, NOMES_DISTRIBUICAO[dist], n, r, semente, status);
#endif
        fflush(csv);   /* grava já: se o processo morrer depois, esta linha fica salva */
    }

    fclose(csv);
    free(original); free(referencia); free(v);
    return houve_erro ? 2 : 0;
}
