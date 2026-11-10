#ifndef VALIDACAO_H
#define VALIDACAO_H

/* Códigos de resultado da validação (vão para o CSV). */
#define VALIDO               0
#define ERRO_NAO_ORDENADO    1
#define ERRO_ELEMENTOS       2

/* Gera a "resposta certa": uma cópia do vetor original ordenada com o qsort
   da biblioteca padrão (referência confiável, independente das nossas implementações).
   Deve ser chamada UMA vez por vetor de entrada, fora da medição de tempo.
   Quem chama libera com free(). */
int *gerar_referencia(const int *original, int n);

/* 1 se v está em ordem não decrescente, 0 caso contrário. */
int esta_ordenado(const int *v, int n);

/* Compara o resultado de um algoritmo com a referência.
   Devolve VALIDO, ERRO_NAO_ORDENADO ou ERRO_ELEMENTOS. */
int validar(const int *resultado, const int *referencia, int n);

#endif
