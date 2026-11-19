"""Análise dos experimentos: estatísticas, razões de crescimento e os 3 gráficos.

Uso (na raiz do projeto):
    python analise/analise.py            -> usa dados/tempos.csv, dados/contagens.csv
                                            e, se existir, dados/estresse_tempos.csv
    python analise/analise.py piloto     -> usa dados/piloto_*.csv (teste)

Gera:
    dados/<prefixo>estatisticas.csv       média, mediana, desvio-padrão, mín, máx,
                                          nº de repetições válidas e contagens, por cenário
    dados/<prefixo>razoes_crescimento.csv mediana(10n) / mediana(n)  (hipótese H6)
    dados/<prefixo>falhas.csv             cenários com timeout / crash / nao_executado / erro
    graficos/<prefixo>grafico1_tempo_x_tamanho.png
    graficos/<prefixo>grafico2_distribuicao.png
    graficos/<prefixo>grafico3_estresse.png
"""

import re
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")                      # gera arquivos, não abre janela
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# ---------------------------------------------------------------------------
# Configuração
# ---------------------------------------------------------------------------
RAIZ = Path(__file__).resolve().parent.parent
DADOS = RAIZ / "dados"
GRAFICOS = RAIZ / "graficos"

# Cor e marcador FIXOS por algoritmo: o mesmo algoritmo tem sempre a mesma
# aparência em todos os gráficos, mesmo que algum esteja ausente nos dados.
# O marcador diferente de cada um permite distinguir as linhas sem depender da cor.
ESTILO = {
    "bubble":        {"cor": "#2a78d6", "marcador": "o", "nome": "BubbleSort"},
    "heap":          {"cor": "#eb6834", "marcador": "s", "nome": "HeapSort"},
    "merge":         {"cor": "#1baf7a", "marcador": "^", "nome": "MergeSort"},
    "quick":         {"cor": "#eda100", "marcador": "D", "nome": "QuickSort"},
    "quick_ingenuo": {"cor": "#e87ba4", "marcador": "v", "nome": "QuickSort ingênuo"},
}
ORDEM_ALG = list(ESTILO)
ORDEM_DIST = ["aleatorio", "crescente", "decrescente", "iguais", "poucos_distintos"]
NOME_DIST = {"aleatorio": "Aleatório", "crescente": "Crescente",
             "decrescente": "Decrescente", "iguais": "Todos iguais",
             "poucos_distintos": "Poucos distintos"}

N_GRAFICO2 = 10_000          # n fixo do gráfico 2 (usa o maior disponível se faltar)

TINTA = "#0b0b0b"            # texto principal
TINTA_2 = "#52514e"          # texto secundário, eixos
GRADE = "#e4e3df"            # grade discreta

plt.rcParams.update({
    "figure.dpi": 150, "savefig.dpi": 200, "font.size": 10,
    "axes.edgecolor": TINTA_2, "axes.labelcolor": TINTA, "axes.titlecolor": TINTA,
    "xtick.color": TINTA_2, "ytick.color": TINTA_2,
    "axes.spines.top": False, "axes.spines.right": False,
    "axes.grid": True, "grid.color": GRADE, "grid.linewidth": 0.8,
    "legend.frameon": False, "lines.linewidth": 2, "lines.markersize": 6,
})


# ---------------------------------------------------------------------------
# Leitura
# ---------------------------------------------------------------------------
def ler_csv(caminho):
    if not caminho.exists():
        return None
    df = pd.read_csv(caminho)
    df["tempo_s"] = pd.to_numeric(df.get("tempo_s"), errors="coerce")
    return df


def ler_limite(prefixo):
    """Lê o tempo limite registrado pelo rodar.sh (padrão: 60 s)."""
    for nome in (f"ambiente_{prefixo.rstrip('_') or 'matriz'}.txt", "ambiente_estresse.txt"):
        arq = DADOS / nome
        if arq.exists():
            m = re.search(r"Limite por repeticao:\s*(\d+)", arq.read_text(encoding="utf-8", errors="ignore"))
            if m:
                return int(m.group(1))
    return 60


# ---------------------------------------------------------------------------
# Estatísticas
# ---------------------------------------------------------------------------
def estatisticas(tempos, contagens):
    ok = tempos[tempos["status"] == "ok"]
    est = (ok.groupby(["algoritmo", "distribuicao", "n"])["tempo_s"]
             .agg(repeticoes="count", media="mean", mediana="median",
                  desvio_padrao="std", minimo="min", maximo="max")
             .reset_index())
    # coeficiente de variação: desvio relativo à média (mostra a estabilidade da medição)
    est["cv_percent"] = 100 * est["desvio_padrao"] / est["media"]

    if contagens is not None:
        cont = contagens[contagens["status"] == "ok"][
            ["algoritmo", "distribuicao", "n", "comparacoes", "trocas"]]
        est = est.merge(cont, on=["algoritmo", "distribuicao", "n"], how="left")
    return ordenar(est)


def ordenar(df, coluna_n="n"):
    df = df.copy()
    df["_a"] = df["algoritmo"].map({a: i for i, a in enumerate(ORDEM_ALG)})
    df["_d"] = df["distribuicao"].map({d: i for i, d in enumerate(ORDEM_DIST)})
    return df.sort_values(["_a", "_d", coluna_n]).drop(columns=["_a", "_d"])


def razoes_crescimento(est):
    """mediana(10n) / mediana(n). Teoria: ~100 para O(n²), ~12,5 para O(n log n)."""
    linhas = []
    for (alg, dist), g in est.groupby(["algoritmo", "distribuicao"]):
        med = g.set_index("n")["mediana"]
        for n in med.index:
            if 10 * n in med.index:
                linhas.append({"algoritmo": alg, "distribuicao": dist,
                               "de_n": n, "para_n": 10 * n,
                               "razao": med[10 * n] / med[n]})
    return ordenar(pd.DataFrame(linhas), "de_n") if linhas else pd.DataFrame()


def falhas(*tabelas):
    partes = [t[t["status"] != "ok"] for t in tabelas if t is not None]
    if not partes:
        return pd.DataFrame()
    f = pd.concat(partes)[["algoritmo", "distribuicao", "n", "status"]]
    return ordenar(f.drop_duplicates())


def inclinacao_loglog(ns, ts):
    """Inclinação da reta em escala log-log: ~2 para O(n²), ~1 para O(n log n)."""
    if len(ns) < 2:
        return None
    return np.polyfit(np.log10(ns), np.log10(ts), 1)[0]


# ---------------------------------------------------------------------------
# Gráficos
# ---------------------------------------------------------------------------
def rotulo_tempo(ax):
    ax.set_ylabel("Tempo (mediana, segundos)")


def grafico1(est, saida):
    """Tempo x tamanho, vetor aleatório, escala log-log."""
    dados = est[est["distribuicao"] == "aleatorio"]
    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    for alg in ORDEM_ALG:
        g = dados[dados["algoritmo"] == alg].sort_values("n")
        if g.empty:
            continue
        s = ESTILO[alg]
        incl = inclinacao_loglog(g["n"], g["mediana"])
        legenda = s["nome"] + (f"  (inclinação ≈ {incl:.2f})" if incl is not None else "")
        ax.plot(g["n"], g["mediana"], color=s["cor"], marker=s["marcador"], label=legenda)
        # rótulo direto no último ponto
        ax.annotate(s["nome"], (g["n"].iloc[-1], g["mediana"].iloc[-1]),
                    xytext=(6, 0), textcoords="offset points",
                    va="center", fontsize=8.5, color=TINTA)
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlabel("Tamanho da entrada n (escala log)")
    rotulo_tempo(ax)
    ax.set_title("Gráfico 1 — Tempo × tamanho da entrada (vetor aleatório)", loc="left")
    ax.text(0, -0.2, "Escala log-log: O(n²) aparece como reta de inclinação ≈ 2; "
            "O(n log n), de inclinação ≈ 1.", transform=ax.transAxes,
            fontsize=8, color=TINTA_2)
    ax.legend(loc="upper left", fontsize=8.5)
    ax.margins(x=0.15)
    fig.tight_layout()
    fig.savefig(saida, bbox_inches="tight")
    plt.close(fig)


def grafico2(est, saida):
    """Distribuição x desempenho, n fixo, barras agrupadas (eixo y em log)."""
    ns = sorted(est["n"].unique())
    n = N_GRAFICO2 if N_GRAFICO2 in ns else max(ns)
    dados = est[est["n"] == n]
    algs = [a for a in ORDEM_ALG if a in set(est["algoritmo"])]
    dists = [d for d in ORDEM_DIST if d in set(est["distribuicao"])]

    fig, ax = plt.subplots(figsize=(8.5, 4.8))
    largura = 0.8 / len(algs)
    x = np.arange(len(dists))
    for i, alg in enumerate(algs):
        s = ESTILO[alg]
        valores = []
        for d in dists:
            linha = dados[(dados["algoritmo"] == alg) & (dados["distribuicao"] == d)]
            valores.append(linha["mediana"].iloc[0] if not linha.empty else np.nan)
        pos = x - 0.4 + largura * (i + 0.5)
        ax.bar(pos, valores, width=largura * 0.88, color=s["cor"],
               label=s["nome"], edgecolor="white", linewidth=1)
        # cenário sem resultado válido (timeout/crash): marca explícita
        for p, v in zip(pos, valores):
            if np.isnan(v):
                ax.annotate("×", (p, 0), xycoords=("data", "axes fraction"),
                            xytext=(0, 4), textcoords="offset points",
                            ha="center", fontsize=11, color=TINTA)
    ax.set_yscale("log")
    ax.set_xlim(-0.5, len(dists) - 0.5)
    ax.set_xticks(x, [NOME_DIST[d] for d in dists])
    ax.grid(axis="x", visible=False)
    rotulo_tempo(ax)
    ax.set_title(f"Gráfico 2 — Desempenho por distribuição (n = {n:,})".replace(",", "."), loc="left")
    ax.text(0, -0.16, "Eixo vertical em escala log. × = sem resultado válido "
            "(timeout, crash ou não executado).", transform=ax.transAxes,
            fontsize=8, color=TINTA_2)
    ax.legend(ncol=len(algs), loc="upper center", bbox_to_anchor=(0.5, -0.2), fontsize=8.5)
    fig.tight_layout()
    fig.savefig(saida, bbox_inches="tight")
    plt.close(fig)


def grafico3(est, falhas_df, limite, saida):
    """Estresse: vetor aleatório até o maior n, com o limite de tempo."""
    dados = est[est["distribuicao"] == "aleatorio"]
    fal = falhas_df[falhas_df["distribuicao"] == "aleatorio"] if not falhas_df.empty else falhas_df
    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    for alg in ORDEM_ALG:
        g = dados[dados["algoritmo"] == alg].sort_values("n")
        f = fal[fal["algoritmo"] == alg] if not fal.empty else fal
        if g.empty and (f is None or f.empty):
            continue
        s = ESTILO[alg]
        ax.plot(g["n"], g["mediana"], color=s["cor"], marker=s["marcador"], label=s["nome"])
        # falhas: marcador vazado sobre a linha do limite, com o tipo de falha escrito
        if f is not None and not f.empty:
            ax.scatter(f["n"], [limite] * len(f), s=70, facecolors="white",
                       edgecolors=s["cor"], linewidths=2, marker=s["marcador"], zorder=3)
            for _, r in f.iterrows():
                ax.annotate(r["status"].replace("_", " "), (r["n"], limite),
                            xytext=(0, 8), textcoords="offset points",
                            ha="center", fontsize=7.5, color=TINTA_2)
    ax.axhline(limite, color=TINTA_2, linestyle="--", linewidth=1.2)
    ax.annotate(f"limite: {limite} s por execução", (0.99, limite),
                xycoords=("axes fraction", "data"), xytext=(0, -12),
                textcoords="offset points", ha="right", fontsize=8, color=TINTA_2)
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlabel("Tamanho da entrada n (escala log)")
    rotulo_tempo(ax)
    ax.set_ylim(top=limite * 5)            # espaço para os rótulos das falhas
    ax.set_title("Gráfico 3 — Teste de estresse (vetor aleatório)", loc="left")
    ax.text(0, -0.2, "Marcadores vazados sobre a linha tracejada: execuções interrompidas "
            "ou não executadas.", transform=ax.transAxes, fontsize=8, color=TINTA_2)
    ax.legend(loc="lower right", fontsize=8.5)
    fig.tight_layout()
    fig.savefig(saida, bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------------------
# Principal
# ---------------------------------------------------------------------------
def main():
    prefixo = "piloto_" if len(sys.argv) > 1 and sys.argv[1] == "piloto" else ""
    tempos = ler_csv(DADOS / f"{prefixo}tempos.csv")
    contagens = ler_csv(DADOS / f"{prefixo}contagens.csv")
    if tempos is None:
        sys.exit(f"Não encontrei {DADOS / (prefixo + 'tempos.csv')}. Rode o rodar.sh antes.")

    # O estresse (n = 1.000.000) fica em arquivos próprios; junta se existirem.
    if not prefixo:
        t_est = ler_csv(DADOS / "estresse_tempos.csv")
        c_est = ler_csv(DADOS / "estresse_contagens.csv")
        if t_est is not None:
            tempos = pd.concat([tempos, t_est], ignore_index=True)
        if c_est is not None and contagens is not None:
            contagens = pd.concat([contagens, c_est], ignore_index=True)

    GRAFICOS.mkdir(exist_ok=True)
    limite = ler_limite(prefixo)

    est = estatisticas(tempos, contagens)
    raz = razoes_crescimento(est)
    fal = falhas(tempos, contagens)

    est.to_csv(DADOS / f"{prefixo}estatisticas.csv", index=False, float_format="%.9g")
    raz.to_csv(DADOS / f"{prefixo}razoes_crescimento.csv", index=False, float_format="%.4g")
    fal.to_csv(DADOS / f"{prefixo}falhas.csv", index=False)

    grafico1(est, GRAFICOS / f"{prefixo}grafico1_tempo_x_tamanho.png")
    grafico2(est, GRAFICOS / f"{prefixo}grafico2_distribuicao.png")
    grafico3(est, fal, limite, GRAFICOS / f"{prefixo}grafico3_estresse.png")

    # Resumo no terminal
    pd.set_option("display.width", 140)
    print("== Mediana dos tempos (s), vetor aleatório ==")
    tab = est[est["distribuicao"] == "aleatorio"].pivot(index="algoritmo", columns="n", values="mediana")
    print(tab.reindex([a for a in ORDEM_ALG if a in tab.index]).to_string(float_format=lambda v: f"{v:.3g}"))
    if not raz.empty:
        print("\n== Razões de crescimento (tempo com 10n / tempo com n), vetor aleatório ==")
        r = raz[raz["distribuicao"] == "aleatorio"]
        print(r[["algoritmo", "de_n", "para_n", "razao"]].to_string(index=False, float_format=lambda v: f"{v:.1f}"))
    print(f"\nCenários com falha: {len(fal)}")
    print(f"Arquivos gerados em {DADOS.name}/ e {GRAFICOS.name}/ (prefixo '{prefixo}')")


if __name__ == "__main__":
    main()
