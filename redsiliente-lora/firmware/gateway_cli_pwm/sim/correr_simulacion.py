"""Ejecuta el firmware en Renode y mide la salida PWM emulada.

Uso:
    python correr_simulacion.py --renode C:/ruta/renode_1.17.1-portable/renode.exe
    python correr_simulacion.py --solo-analizar      # reanaliza resultados/ sin emular

Requisitos: haber compilado con "pio run" (usa .pio/build/lrwan1_cli/firmware.elf),
Renode 1.17.1 y matplotlib para la figura.

Plataforma: Renode modela el STM32L072 con los timers a 10 MHz fijos (atributo del
constructor, que no se puede cambiar despues: "timer3 Frequency" solo cambia el contador
principal, no los comparadores de los canales). El firmware deja los timers a 32 MHz
(HSI16 x4 /2, APB /1), asi que este script genera resultados/stm32l072_32mhz.repl a
partir de platforms/cpus/stm32l071.repl + stm32l072.repl de Renode cambiando solo
"frequency: 10000000" -> "frequency: 32000000" en los timers.

Salidas en resultados/:
    uart.txt        lo que el firmware escribio por LPUART1 (la "terminal")
    eventos.csv     flancos de PB5 con tiempo virtual y marcas de cada caso
    renode.log      log de Renode (avisos del modelo)
    mediciones.md   tablas de periodo y tiempos medidos
    pwm_simulado.png  forma de onda de PB5 en los casos 4 y 6
"""
import argparse
import csv
import os
import re
import subprocess
import sys
from pathlib import Path

SIM = Path(__file__).resolve().parent
RES = SIM / "resultados"
SETTLE_S = 0.003  # se ignoran los primeros 3 ms tras cada comando


TIMER_FREQ_MODELO = "frequency: 10000000"
TIMER_FREQ_FIRMWARE = "frequency: 32000000"


def generar_plataforma(renode: Path) -> None:
    cpus = renode.parent / "platforms" / "cpus"
    base = (cpus / "stm32l071.repl").read_text(encoding="utf-8")
    extra = (cpus / "stm32l072.repl").read_text(encoding="utf-8")
    extra = extra.replace('using "./stm32l071.repl"', "")
    if base.count(TIMER_FREQ_MODELO) != 6:  # TIM2, 3, 21, 22, 6, 7
        sys.exit("stm32l071.repl no tiene la forma esperada (otra version de Renode?)")
    cabecera = ("// Generado por correr_simulacion.py desde stm32l071.repl + stm32l072.repl de\n"
                "// Renode; unico cambio: timers a 32 MHz como los deja el firmware.\n")
    (RES / "stm32l072_32mhz.repl").write_text(
        cabecera + base.replace(TIMER_FREQ_MODELO, TIMER_FREQ_FIRMWARE) + extra, encoding="utf-8")


def correr_renode(renode: Path) -> None:
    RES.mkdir(exist_ok=True)
    generar_plataforma(renode)
    for nombre in ("uart.txt", "eventos.csv"):
        (RES / nombre).unlink(missing_ok=True)  # Renode renombra los viejos a .1
    plantilla = (SIM / "simulacion.resc").read_text(encoding="utf-8")
    script = RES / "simulacion_rutas.resc"
    script.write_text(plantilla.replace("{SIM}", SIM.as_posix()), encoding="utf-8")
    proc = subprocess.run(
        [str(renode), "--console", "--disable-gui", "-e", f"include @{script.as_posix()}"],
        cwd=renode.parent, capture_output=True, text=True, encoding="utf-8", errors="replace",
        timeout=900)
    log = re.sub(r"\x1b\[[0-9;]*m", "", proc.stdout + proc.stderr)
    (RES / "renode.log").write_text(log, encoding="utf-8")
    if not (RES / "eventos.csv").exists():
        sys.exit("Renode no genero eventos.csv; revisar resultados/renode.log")


def leer_eventos():
    marcas, flancos = [], []
    with open(RES / "eventos.csv", encoding="utf-8") as f:
        for fila in csv.DictReader(f):
            t = float(fila["t_s"])
            if fila["tipo"] == "marca":
                marcas.append((t, fila["valor"]))
            else:
                flancos.append((t, int(fila["valor"])))
    return marcas, flancos


def medir(flancos, t0, t1):
    """Periodo y tiempo en alto promedio entre t0+SETTLE y t1."""
    tramo = [(t, v) for t, v in flancos if t0 + SETTLE_S <= t < t1]
    subidas = [t for t, v in tramo if v == 1]
    if len(subidas) < 2:
        nivel = [v for t, v in flancos if t < t1]
        return {"ciclos": 0, "nivel_fijo": nivel[-1] if nivel else 0}
    periodos, altos = [], []
    for a, b in zip(subidas, subidas[1:]):
        bajada = next((t for t, v in tramo if v == 0 and a < t < b), None)
        if bajada is not None:
            periodos.append(b - a)
            altos.append(bajada - a)
    periodo = sum(periodos) / len(periodos)
    alto = sum(altos) / len(altos)
    return {"ciclos": len(periodos), "periodo_s": periodo, "alto_s": alto}


def fmt_t(s):
    return f"{s * 1e3:.4f} ms" if s >= 1e-3 else f"{s * 1e6:.2f} µs"


def analizar() -> str:
    marcas, flancos = leer_eventos()
    filas = []
    for i, (t0, nombre) in enumerate(marcas):
        if nombre in ("fin", "cambio_de_curva", "preparar_boton"):
            continue
        t1 = marcas[i + 1][0] if i + 1 < len(marcas) else t0 + 0.02
        filas.append((nombre, medir(flancos, t0, t1)))

    lineas = [
        "| Caso | Ciclos | Periodo | Frecuencia | Tiempo alto | Tiempo bajo | Duty |",
        "|---|---|---|---|---|---|---|",
    ]
    for nombre, m in filas:
        if m["ciclos"] == 0:
            lineas.append(f"| `{nombre}` | 0 | — | — | fijo en {m['nivel_fijo']} | — | — |")
            continue
        p, alto = m["periodo_s"], m["alto_s"]
        lineas.append(
            f"| `{nombre}` | {m['ciclos']} | {fmt_t(p)} | {1 / p:.3f} Hz | "
            f"{fmt_t(alto)} | {fmt_t(p - alto)} | {100 * alto / p:.2f} % |")
    tabla = "\n".join(lineas)
    (RES / "mediciones.md").write_text(tabla + "\n", encoding="utf-8")
    figura(marcas, flancos)
    return tabla


def figura(marcas, flancos):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib no instalado: se omite la figura")
        return
    t_marca = dict((n, t) for t, n in marcas)
    casos = [("caso4_set_duty_50", "set frequency 500 + set duty 50"),
             ("caso6_set_duty_25", "set frequency 1000 + set duty 25")]
    fig, ejes = plt.subplots(len(casos), 1, figsize=(12, 5.5))
    for ax, (clave, titulo) in zip(ejes, casos):
        t0 = t_marca[clave] + 0.005
        t1 = t0 + 0.005
        nivel = [v for t, v in flancos if t <= t0][-1:] or [0]
        xs, ys = [0.0], [nivel[0]]
        for t, v in flancos:
            if t0 < t < t1:
                xs += [(t - t0) * 1e3, (t - t0) * 1e3]
                ys += [ys[-1], v]
        xs.append(5.0)
        ys.append(ys[-1])
        ax.plot(xs, ys, drawstyle="default", lw=1.5, color="#1f4e79")
        ax.set_ylim(-0.3, 1.4)
        ax.set_yticks([0, 1])
        ax.set_xlim(0, 5)
        ax.grid(ls=":", alpha=0.6)
        ax.set_title(f"PB5 emulado — {titulo}", fontsize=10)
        ax.set_ylabel("PB5")
    ejes[-1].set_xlabel("tiempo virtual [ms] desde 5 ms después del comando")
    fig.suptitle("Simulación en Renode 1.17.1 con el firmware compilado "
                 "(no es una captura de Logic 2 sobre la placa)", fontsize=10)
    fig.tight_layout()
    fig.savefig(RES / "pwm_simulado.png", dpi=130)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--renode", type=Path, default=os.environ.get("RENODE"))
    parser.add_argument("--solo-analizar", action="store_true")
    args = parser.parse_args()
    if not args.solo_analizar:
        if not args.renode or not Path(args.renode).exists():
            sys.exit("Indicar --renode <ruta a renode.exe> (o la variable RENODE)")
        correr_renode(Path(args.renode))
    print(analizar())


if __name__ == "__main__":
    main()
