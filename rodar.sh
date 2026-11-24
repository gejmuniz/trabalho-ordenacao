#!/usr/bin/env bash
# Roda a matriz de experimentos e grava os CSVs em dados/.
#
# Uso (na raiz do projeto, no Git Bash):
#   ./rodar.sh piloto     -> n = 100 e 1.000, 2 repetições (confere se tudo funciona)
#   ./rodar.sh matriz     -> matriz completa, n = 100 a 100.000
#   ./rodar.sh estresse   -> aleatório com n = 1.000.000
#
# Status gravados por ESTE script (os demais são gravados pelo próprio medir):
#   timeout        -> o cenário passou do tempo limite e foi interrompido
#   crash          -> o programa morreu sozinho (ex.: estouro de pilha)
#   nao_executado  -> não rodado: o mesmo algoritmo/distribuição já falhou
#                     (timeout ou crash) com um n menor

set -u

# ===================== CONFIGURAÇÃO (decisões da dupla) =====================

# Algoritmos a medir (os nomes são os da tabela ALGORITMOS[] do main.c).
ALGORITMOS="bubble heap merge quick quick_ingenuo"

DISTRIBUICOES="aleatorio crescente decrescente iguais poucos_distintos"

SEMENTE=2026

# Tempo limite POR REPETIÇÃO, em segundos. O cenário inteiro tem LIMITE_S x repetições.
LIMITE_S=60

# Repetições por tamanho.
repeticoes_para() {
    case "$1" in
        100|1000|10000) echo 10 ;;
        100000)         echo 5  ;;
        *)              echo 3  ;;
    esac
}

# Opções de compilação (registradas em dados/ambiente_<modo>.txt).
CFLAGS="-O2 -Wall -Wextra -std=c11"

# No Windows, o ligador grava a data/hora da compilação dentro do .exe; cada
# compilação gera então um arquivo "novo", que o Controle Inteligente de
# Aplicativos precisa avaliar de novo (e às vezes bloqueia). Sem a data, o mesmo
# código gera sempre o mesmo .exe: depois de liberado uma vez, continua liberado.
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) LDFLAGS="-Wl,--no-insert-timestamp" ;;
    *)                    LDFLAGS="" ;;
esac

# ============================================================================

MODO="${1:-}"
case "$MODO" in
    piloto)   TAMANHOS="100 1000";   DISTS="$DISTRIBUICOES"; PREFIXO="piloto_" ;;
    matriz)   TAMANHOS="100 1000 10000 100000"; DISTS="$DISTRIBUICOES"; PREFIXO="" ;;
    estresse) TAMANHOS="1000000";    DISTS="aleatorio";      PREFIXO="estresse_" ;;
    *) echo "Uso: ./rodar.sh piloto | matriz | estresse"; exit 1 ;;
esac

RAIZ="$(cd "$(dirname "$0")" && pwd)"
SRC="$RAIZ/src"
DADOS="$RAIZ/dados"
TEMPOS="$DADOS/${PREFIXO}tempos.csv"
CONTAGENS="$DADOS/${PREFIXO}contagens.csv"
AMBIENTE="$DADOS/ambiente_${MODO}.txt"
ERROS="$DADOS/erros_${MODO}.log"       # mensagens de erro dos programas (antes eram descartadas)
mkdir -p "$DADOS"

# Piloto pode ser refeito à vontade; os dados de verdade nunca são sobrescritos.
if [ "$MODO" = "piloto" ]; then
    rm -f "$TEMPOS" "$CONTAGENS" "$ERROS"
elif [ -e "$TEMPOS" ] || [ -e "$CONTAGENS" ]; then
    echo "ERRO: $TEMPOS ou $CONTAGENS já existe."
    echo "Para refazer, apague ou renomeie esses arquivos antes."
    exit 1
fi

# ---------- 1. Compilação ----------
echo "== Compilando =="
cd "$SRC" || exit 1
gcc $CFLAGS          -o medir  main.c algoritmos.c dados.c validacao.c $LDFLAGS || exit 1
gcc $CFLAGS -DCONTAR -o contar main.c algoritmos.c dados.c validacao.c $LDFLAGS || exit 1
MEDIR="./medir";  [ -f medir.exe ]  && MEDIR="./medir.exe"
CONTAR="./contar"; [ -f contar.exe ] && CONTAR="./contar.exe"

# Teste de partida: no Windows, o antivírus pode bloquear um .exe recém-compilado
# ("Permission denied"). Sem este teste, o bloqueio seria registrado como
# "crash" em todos os cenários. Tenta por até ~2 minutos antes de desistir.
echo "== Testando os executaveis =="
TESTE_CSV="$DADOS/.teste_partida.csv"
executaveis_rodam() {   # 0 (sucesso) se medir e contar conseguem rodar um caso minúsculo
    rm -f "$TESTE_CSV" "$TESTE_CSV.c"
    $MEDIR bubble aleatorio 10 1 1 "$TESTE_CSV" 2>>"$ERROS" && \
    $CONTAR bubble aleatorio 10 1 "$TESTE_CSV.c" 2>>"$ERROS"
    local r=$?
    rm -f "$TESTE_CSV" "$TESTE_CSV.c"
    return $r
}
pronto=""
for tentativa in $(seq 1 20); do
    if executaveis_rodam; then
        pronto="sim"; break
    fi
    echo "  executaveis ainda nao liberados pelo Windows (tentativa $tentativa de 20); aguardando 6 s..."
    sleep 6
done
if [ -z "$pronto" ]; then
    echo "ERRO: nao foi possivel executar $MEDIR. Nenhum dado foi gravado."
    echo "Rode manualmente para ver a mensagem:  cd src && $MEDIR bubble aleatorio 10 1 1 teste.csv"
    echo "Mensagens registradas em: $ERROS"
    exit 1
fi
echo "  ok"

# ---------- 2. Registro do ambiente (vai para a Metodologia) ----------
{
    echo "Modo: $MODO"
    echo "Inicio: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Commit: $(git -C "$RAIZ" rev-parse --short HEAD 2>/dev/null || echo desconhecido)"
    if [ -n "$(git -C "$RAIZ" status --porcelain -- src 2>/dev/null)" ]; then
        echo "ATENCAO: src/ tem alteracoes sem commit -- o hash acima nao descreve exatamente o codigo usado"
    fi
    echo "Compilador: $(gcc --version | head -1)"
    echo "Opcoes: $CFLAGS $LDFLAGS"
    echo "Sistema: $(uname -a)"
    echo "Semente: $SEMENTE"
    echo "Limite por repeticao: ${LIMITE_S} s"
    echo "Algoritmos: $ALGORITMOS"
} > "$AMBIENTE"
cat "$AMBIENTE"

# Grava uma linha de falha (timeout/crash/nao_executado) nos dois CSVs.
registrar_falha() {   # algoritmo distribuicao n status
    [ -s "$TEMPOS" ] || echo "algoritmo,distribuicao,n,repeticao,semente,tempo_s,status" > "$TEMPOS"
    [ -s "$CONTAGENS" ] || echo "algoritmo,distribuicao,n,semente,comparacoes,trocas,status" > "$CONTAGENS"
    echo "$1,$2,$3,,$SEMENTE,,$4" >> "$TEMPOS"
    echo "$1,$2,$3,$SEMENTE,,,$4" >> "$CONTAGENS"
}

# Traduz o código de saída do processo em status.
status_de() {
    case "$1" in
        0)   echo ok ;;
        2)   echo erro_validacao ;;
        124) echo timeout ;;
        1)   echo erro_uso ;;
        126|127) echo erro_execucao ;;   # programa não pôde ser iniciado (bloqueado / não encontrado)
        *)   echo crash ;;
    esac
}

# ---------- 3. Execução ----------
echo
echo "== Executando ($MODO) =="
for alg in $ALGORITMOS; do
    for dist in $DISTS; do
        falhou=""    # vira "sim" quando este algoritmo/distribuição falha
        for n in $TAMANHOS; do
            if [ -n "$falhou" ]; then
                printf "  %-14s %-17s n=%-8s nao_executado\n" "$alg" "$dist" "$n"
                registrar_falha "$alg" "$dist" "$n" nao_executado
                continue
            fi

            reps=$(repeticoes_para "$n")
            [ "$MODO" = "piloto" ] && reps=2
            limite=$((LIMITE_S * reps))

            # Tempo
            inicio=$(date +%s)
            timeout "$limite" $MEDIR "$alg" "$dist" "$n" "$reps" "$SEMENTE" "$TEMPOS" 2>>"$ERROS"
            cod=$?
            st=$(status_de $cod)
            dur=$(( $(date +%s) - inicio ))

            # Códigos 126/127 podem ser bloqueio do Windows OU, no Windows, um crash
            # real (ex.: estouro de pilha) relatado com esse código. Para distinguir,
            # testa de novo se o medir consegue rodar: se consegue, foi crash do algoritmo.
            if [ "$st" = "erro_execucao" ]; then
                if executaveis_rodam; then
                    st="crash"
                else
                    echo "ERRO: o medir nao pode ser iniciado (codigo $cod) em $alg/$dist/n=$n."
                    echo "Execucao interrompida para nao registrar falsos 'crash'. Veja $ERROS."
                    echo "Apague $TEMPOS e $CONTAGENS antes de rodar de novo."
                    exit 1
                fi
            fi
            if [ "$st" = "erro_uso" ]; then
                echo "ERRO: o medir nao reconhece '$alg' ou '$dist' -- confira ALGORITMOS no topo do script"
                exit 1
            fi
            if [ "$st" = "timeout" ] || [ "$st" = "crash" ]; then
                printf "  %-14s %-17s n=%-8s %s (%ss, codigo %s)\n" "$alg" "$dist" "$n" "$st" "$dur" "$cod"
                echo "$alg $dist n=$n: $st (codigo de saida $cod)" >> "$ERROS"
                registrar_falha "$alg" "$dist" "$n" "$st"
                falhou="sim"
                continue
            fi

            # Contagem (uma execução; só se a medição de tempo foi bem-sucedida)
            timeout "$LIMITE_S" $CONTAR "$alg" "$dist" "$n" "$SEMENTE" "$CONTAGENS" 2>>"$ERROS"
            stc=$(status_de $?)
            [ "$stc" = "erro_execucao" ] && executaveis_rodam && stc="crash"
            if [ "$stc" = "timeout" ] || [ "$stc" = "crash" ] || [ "$stc" = "erro_execucao" ]; then
                echo "$alg,$dist,$n,$SEMENTE,,,$stc" >> "$CONTAGENS"
            fi

            printf "  %-14s %-17s n=%-8s %s (%ss, %s rep.)\n" "$alg" "$dist" "$n" "$st" "$dur" "$reps"
        done
    done
done

echo "Fim: $(date '+%Y-%m-%d %H:%M:%S')" >> "$AMBIENTE"
echo
echo "== Pronto =="
echo "Tempos:    $TEMPOS"
echo "Contagens: $CONTAGENS"
echo "Ambiente:  $AMBIENTE"
echo "Erros:     $ERROS"
