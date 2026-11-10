#ifndef ALGORITMOS_H
#define ALGORITMOS_H

/* ---------- Contadores de operações ----------
   Os contadores ajudam a EXPLICAR os tempos (o tempo diz "quanto demorou",
   os contadores dizem "por quê").

   Eles só existem quando o código é compilado com -DCONTAR.
   Motivo: incrementar um contador também custa tempo. Então:
     - execuções de TEMPO são compiladas sem -DCONTAR (medição limpa);
     - execuções de CONTAGEM são compiladas com -DCONTAR.
   Como os algoritmos são determinísticos, a contagem é a mesma em toda
   repetição: basta uma execução de contagem por cenário. */

extern long long comparacoes;   /* comparações entre elementos do vetor */
extern long long trocas;        /* trocas (ou cópias, no MergeSort) */

void zerar_contadores(void);

#ifdef CONTAR
  #define CONTA_COMPARACAO() (comparacoes++)
  #define CONTA_TROCA()      (trocas++)
#else
  #define CONTA_COMPARACAO() ((void)0)
  #define CONTA_TROCA()      ((void)0)
#endif

/* ---------- Algoritmos ----------
   Todos com a mesma assinatura: ordenam v[0..n-1] em ordem crescente, no lugar. */

void bubble_sort(int *v, int n);

#endif
