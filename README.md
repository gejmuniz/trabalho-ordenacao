# Comparação Experimental: Algoritmos de Classificação

Trabalho final de **INF 01203 – Estruturas de Dados** (UFRGS, turma A, Profa. Renata Galante).

Comparativo experimental entre **BubbleSort, QuickSort, MergeSort e HeapSort**, em diferentes distribuições de entrada e tamanhos de 100 a 1.000.000 elementos, analisando desempenho, escalabilidade, uso de recursos e características dos algoritmos.

---

## 1. Estrutura do repositório

```
trabalho-ordenacao/
├── src/          código C (algoritmos, gerador de dados, validador, testes, medição)
├── dados/        CSVs com as execuções individuais
├── analise/      scripts Python (estatísticas e gráficos)
├── graficos/     imagens geradas pelos scripts
└── relatorios/   relatório final
```

| Arquivo em `src/` | O que faz |
|---|---|
| `dados.c/.h` | Gera os vetores de entrada (5 distribuições) a partir de uma semente |
| `validacao.c/.h` | Confere se o resultado está ordenado **e** contém exatamente os mesmos elementos da entrada |
| `algoritmos.c/.h` | Os algoritmos de ordenação e os contadores de comparações/trocas |
| `teste_dados.c` | Testes do gerador e do validador |
| `teste_algoritmos.c` | Testes de correção dos algoritmos (todas as distribuições + casos de borda) |

---

## 2. Como compilar e rodar (Windows)

### Requisitos
- **Git for Windows** (git-scm.com), com as opções padrão.
- **gcc via MSYS2** (msys2.org):
  1. Instalar o MSYS2 e abrir o terminal **MSYS2 UCRT64**.
  2. `pacman -S mingw-w64-ucrt-x86_64-gcc`
  3. Adicionar `C:\msys64\ucrt64\bin` ao **Path** do usuário (Variáveis de Ambiente), para o `gcc` funcionar também no Git Bash e no VS Code.
  4. Fechar e reabrir o terminal; conferir com `gcc --version`.

### ⚠️ Não coloque o projeto dentro do OneDrive
O OneDrive bloqueia a execução dos programas recém-compilados (`Permission denied`) e a sincronização em segundo plano atrapalharia a medição de tempo. Use uma pasta local, por exemplo `C:\Users\<usuario>\projetos\`.

### Compilar e testar
```bash
cd src

# testes do gerador e do validador
gcc -O2 -Wall -Wextra -std=c11 -o teste_dados teste_dados.c dados.c validacao.c
./teste_dados

# testes dos algoritmos (com contadores ligados)
gcc -O2 -Wall -Wextra -std=c11 -DCONTAR -o teste_algoritmos teste_algoritmos.c algoritmos.c dados.c validacao.c
./teste_algoritmos
```
Os dois devem terminar com `TODOS OS TESTES PASSARAM`. Como o gerador usa semente fixa, os números de comparações e trocas são **idênticos em qualquer computador** (ex.: Bubble aleatório n=1000 → 252344 trocas).

### Fluxo de trabalho no Git
- Antes de começar: `git pull`
- Ao terminar: `git add .` → `git commit -m "descrição"` → `git push`
- Para evitar conflitos, combinem quem está mexendo em qual arquivo antes de começar.
- Ao concluir um item do checklist (seção 6), troque `[ ]` por `[x]` e faça um commit.

---

## 3. Decisões de projeto (e por quê)

### Gerador de dados (`dados.c`)
- **Gerador pseudoaleatório próprio (splitmix64)**, não `rand()`: no Windows `RAND_MAX = 32767` (insuficiente para embaralhar 1.000.000 posições) e `rand()` muda entre sistemas. Com o gerador próprio, a mesma semente gera o mesmo vetor em qualquer computador.
- **Aleatório sem repetição** = valores 1..n embaralhados com **Fisher-Yates**.
- **Poucos distintos** = valores sorteados em `[0, 10)` (constante `K_POUCOS_DISTINTOS`).
- **Todos iguais** = todos os elementos valem 42.
- A semente é aplicada dentro de `gerar_vetor`, então o vetor depende só de (distribuição, n, semente).

### Validação (`validacao.c`)
- A referência é a entrada ordenada com o `qsort` da biblioteca padrão (independente das nossas implementações), gerada **uma vez por vetor, fora da medição**.
- Execução válida = resultado ordenado **e** idêntico à referência. Isso pega também o erro "ordenou, mas perdeu/duplicou elementos".

### Contadores de comparações e trocas
- Só existem quando o código é compilado com **`-DCONTAR`**, porque incrementar contadores também custa tempo.
- **Execuções de tempo**: compiladas **sem** `-DCONTAR` (medição limpa).
- **Execuções de contagem**: compiladas **com** `-DCONTAR`. Como os algoritmos são determinísticos, basta uma execução por cenário.
- No MergeSort, "trocas" conta as **cópias** de elementos (ele não faz trocas).

### Implementações
Todas têm a mesma assinatura: `void nome(int *v, int n)`.

| Algoritmo | Escolhas | Motivo |
|---|---|---|
| **BubbleSort** ✅ | Flag "houve troca" + limite que encolhe; troca só com `>` | Flag dá melhor caso O(n) no vetor ordenado; `>` mantém a estabilidade |
| **HeapSort** ✅ | Construção bottom-up (Floyd, O(n)); `sift_down` **iterativo**; índices a partir de 0 | Sem recursão nenhuma (contraste com Quick/Merge) |
| **MergeSort** ⏳ | Top-down recursivo; vetor auxiliar alocado **uma vez**; `<=` no merge; **sem** a otimização "pular merge se já ordenado" | Alocar a cada chamada mediria o `malloc`; `<=` garante estabilidade; sem a otimização ele mantém o comportamento teórico de tempo igual em qualquer distribuição |
| **QuickSort melhorado** ⏳ | Pivô **mediana de três** + partição de **Hoare** | Versão "oficial" na comparação. Mediana de três evita o pior caso em vetores ordenados; Hoare divide elementos iguais ao meio |
| **QuickSort ingênuo** ⏳ | Pivô = último elemento + partição de **Lomuto** | Bônus: demonstra o pior caso O(n²) em vetores ordenados/iguais e o estouro de pilha (crash) no teste de estresse |

### Medição (a implementar)
- Cada algoritmo recebe uma **cópia** do mesmo vetor original.
- Mede-se **só** a chamada do algoritmo (fora: geração, cópia, validação, escrita no CSV).
- Relógio de alta precisão (`clock_gettime(CLOCK_MONOTONIC)` ou `QueryPerformanceCounter`).
- Cada cenário roda em **processo separado**, com limite de tempo: se o Quick ingênuo estourar a pilha, só aquele processo morre.
- Compilação com `gcc -O2`.

### Pendente de decisão
- [ ] Número de repetições por tamanho (sugestão: 10 para n ≤ 10.000; 5 para 100.000; 3 para 1.000.000).
- [ ] Tempo limite por execução no teste de estresse (sugestão: 60 s).
- [ ] Manter a pilha padrão do Windows (1 MB) ou aumentá-la na compilação. Afeta quando o Quick ingênuo estoura — bom tema para a pergunta "o limite depende do algoritmo, da implementação ou do computador?".
- [ ] A matriz do enunciado tem "10000" na segunda coluna do "Ordenado Crescente" — provável erro de digitação de 1.000; confirmar com a professora (por ora tratamos como 1.000).
- [ ] O `malloc` do vetor auxiliar do MergeSort entra na medição? (sugestão: sim, é custo do algoritmo; documentar na Metodologia).

---

## 4. Formato dos dados (CSV)

Uma linha por **execução individual** (a especificação pede as execuções individuais; médias e medianas são calculadas depois, no Python). Separador `,`, ponto decimal `.`, com cabeçalho.

### `dados/tempos.csv` — execuções de tempo (compiladas sem `-DCONTAR`)
```
algoritmo,distribuicao,n,repeticao,semente,tempo_s,status
bubble,aleatorio,1000,1,2026,0.00123456,ok
bubble,aleatorio,1000,2,2026,0.00119870,ok
quick_ingenuo,crescente,100000,1,2026,,crash
bubble,aleatorio,1000000,1,2026,,nao_executado
```

### `dados/contagens.csv` — uma execução por cenário (compilada com `-DCONTAR`)
```
algoritmo,distribuicao,n,semente,comparacoes,trocas,status
bubble,aleatorio,1000,2026,499409,252344,ok
heap,iguais,1000,2026,2994,999,ok
```

### Valores possíveis
| Coluna | Valores |
|---|---|
| `algoritmo` | `bubble`, `heap`, `merge`, `quick`, `quick_ingenuo` |
| `distribuicao` | `aleatorio`, `crescente`, `decrescente`, `iguais`, `poucos_distintos` |
| `n` | `100`, `1000`, `10000`, `100000`, `1000000` |
| `tempo_s` | segundos (float); **vazio** quando `status` ≠ `ok` |
| `status` | `ok`, `erro_validacao`, `timeout` (passou do limite), `crash` (ex.: estouro de pilha), `nao_executado` (estimativa acima do limite, não rodado) |

O script de análise deve **separar** as execuções `ok` das demais: as outras não entram nas médias, mas aparecem no relatório (testes de estresse) e no Gráfico 3.

---

## 5. Hipóteses (formuladas ANTES dos experimentos)

**Distribuição dos dados**
- **H1** — BubbleSort é o mais sensível à distribuição: O(n) no vetor crescente (uma passada, 0 trocas), pior caso no decrescente (~n²/2 trocas).
- **H2** — QuickSort com pivô fixo degrada para O(n²) em vetores ordenados e pode estourar a pilha com n grande; com mediana de três, não.
- **H3** — Todos iguais: Bubble O(n); Quick/Lomuto O(n²); Quick/Hoare O(n log n); Heap muito rápido (o `sift_down` para logo); Merge igual ao aleatório.
- **H4** — MergeSort é o menos sensível à distribuição (sempre divide ao meio).
- **H5** — Poucos distintos fica entre "aleatório" e "todos iguais"; o Quick sofre se tratar mal os iguais.

**Tamanho da entrada**
- **H6** — Ao multiplicar n por 10: O(n²) fica ~100× mais lento; O(n log n) fica ~12–13× mais lento (efeitos de cache podem aumentar isso).
- **H7** — Com n = 100 as diferenças são pequenas e próximas do ruído; vencer em n pequeno não prova ser a melhor escolha.
- **H8** — Em n grande e vetor aleatório: Quick < Merge < Heap ≪ Bubble (Heap aproveita mal o cache).

**Testes de estresse**
- **H9** — BubbleSort é o primeiro a passar do limite (~10¹⁰ operações com n = 100.000); os demais ordenam 1.000.000 aleatório em menos de 1 s.
- **H10** — Os limites dependem de algoritmo (complexidade), implementação (pivô, recursão) e computador (tamanho da pilha, RAM, CPU).

**Já confirmadas nos testes de correção (n = 1000):**
- H1: Bubble crescente = 999 comparações / 0 trocas; decrescente = 499.500 / 499.500.
- Bubble aleatório: 252.344 trocas ≈ n(n−1)/4 → no Bubble, cada troca desfaz exatamente uma **inversão**.
- H3/H4 (Heap): ~15–18 mil comparações em todas as distribuições, mas só 2.994 com todos iguais (~3n).
- Heap decrescente: **0 trocas** na fase de construção (já é um heap máximo). O pior caso da construção é o **crescente** (992 trocas) — o oposto do Bubble.

---

## 6. Checklist do trabalho

### Código
- [x] Gerador de dados e validador, com testes
- [x] BubbleSort
- [x] HeapSort
- [x] MergeSort
- [x] QuickSort (melhorado e ingênuo)
- [x] Programa de medição (`main.c`) e script que roda a matriz de experimentos

### Experimentos
- [x] Fechar os itens "Pendente de decisão" (seção 3)
- [x] Piloto (n = 100 e 1.000)
- [x] Matriz completa (até 100.000)
- [x] Teste de estresse (aleatório até 1.000.000)
- [x] Documentar o ambiente: processador, RAM, versão do Windows, versão do gcc, opções de compilação, hash do commit usado

### Análise
- [x] Script Python (pandas + matplotlib) que lê `tempos.csv` e `contagens.csv` e calcula média, mediana e desvio-padrão por cenário — pode ser feito antes dos dados reais, com um CSV de exemplo no formato da seção 4
- [x] Gráfico 1 — tempo × tamanho (sugestão: escala log-log; O(n²) aparece com inclinação ~2, O(n log n) com ~1)
- [x] Gráfico 2 — distribuição × desempenho, n fixo (sugestão: barras agrupadas, n = 10.000)
- [x] Gráfico 3 — estresse: tempos no aleatório crescendo, com o limite de tempo como linha horizontal
- [ ] Tabela-síntese final comparando os quatro algoritmos

### Relatório
- [ ] 1. Introdução
- [ ] 2. Algoritmos (implementações, características teóricas, referências)
- [ ] 3. Metodologia experimental
- [ ] 4. Resultados
- [ ] 5. Análise comparativa
- [ ] 6. Testes de estresse
- [ ] 7. Discussão (confrontar com as hipóteses da seção 5)
- [ ] 8. Conclusões ("existe um algoritmo vencedor em todos os cenários?")
- [ ] 9. Referências
- [ ] 10. Declaração do uso de IA
- [ ] 11. Apêndice (compilação, execução e dados brutos)

### Entrega e apresentação
- [ ] Os dois leram e entenderam todo o código e a análise (a professora pode perguntar a qualquer um)
- [ ] Entregar código-fonte, CSVs e relatório
- [ ] Preencher o formulário coletivo
- [ ] Preparar a discussão em aula (20% da nota)

---

## 7. Origem do código e uso de IA

- Algoritmos baseados nos slides da disciplina e em Cormen et al., *Algoritmos: Teoria e Prática* (BubbleSort cap. 2, HeapSort cap. 6, MergeSort cap. 2, QuickSort cap. 7), com adaptações descritas na seção 3.
- O código foi desenvolvido com auxílio de IA (Claude, da Anthropic) para estruturação, implementação e testes, sob orientação e revisão da dupla. Cada um deve registrar aqui também o uso de IA na sua parte, para a Declaração de uso de IA do relatório.
