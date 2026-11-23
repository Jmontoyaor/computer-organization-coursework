"""Genera las figuras del README de redsiliente-lora (img/*.png).

Uso:  python tools/generar_figuras.py      (necesita matplotlib)

Los valores de pines, tiempos y parametros salen del firmware de referencia
(OMANROQUI/Redsiliente, carpetas firmware/node_reporter y firmware/gateway_receiver).
"""
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

IMG = Path(__file__).resolve().parent.parent / "img"
IMG.mkdir(exist_ok=True)

AZUL, VERDE, NARANJA, GRIS, ROJO, MORADO = (
    "#dbe8f7", "#dcf0dc", "#fde5c8", "#eeeeee", "#f8d4d4", "#e6dcf3")


# --------------------------------------------------------------------------
#  Tiempo en el aire LoRa (Semtech AN1200.13 / hoja de datos SX1276 sec. 4.1.1.7)
# --------------------------------------------------------------------------
def lora_toa_ms(payload, sf=7, bw=125e3, cr=1, preamble=8, header=True, crc=True):
    tsym = (2 ** sf) / bw * 1000
    de = 1 if tsym > 16 else 0
    ih = 0 if header else 1
    num = 8 * payload - 4 * sf + 28 + 16 * crc - 20 * ih
    n_payload = 8 + max(math.ceil(num / (4 * (sf - 2 * de))) * (cr + 4), 0)
    t_pre = (preamble + 4.25) * tsym
    return t_pre + n_payload * tsym, tsym, n_payload


# --------------------------------------------------------------------------
#  Utilidades de dibujo
# --------------------------------------------------------------------------
def caja(ax, x, y, w, h, texto, color=GRIS, fs=9, bold=False, borde="#333333", lw=1.2):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.08",
                                fc=color, ec=borde, lw=lw))
    ax.text(x + w / 2, y + h / 2, texto, ha="center", va="center", fontsize=fs,
            fontweight="bold" if bold else "normal")


def titulo_caja(ax, x, y, w, h, texto, color, fs=10):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.12",
                                fc=color, ec="#555555", lw=1.5, alpha=0.55))
    ax.text(x + 0.12, y + h - 0.12, texto, ha="left", va="top", fontsize=fs, fontweight="bold")


def flecha(ax, p1, p2, texto="", color="#222222", fs=8, both=False, rad=0.0, off=(0, 0.12),
           ls="-"):
    estilo = "<|-|>" if both else "-|>"
    ax.add_patch(FancyArrowPatch(p1, p2, arrowstyle=estilo, mutation_scale=12, color=color,
                                 lw=1.3, connectionstyle=f"arc3,rad={rad}", linestyle=ls))
    if texto:
        mx, my = (p1[0] + p2[0]) / 2 + off[0], (p1[1] + p2[1]) / 2 + off[1]
        ax.text(mx, my, texto, ha="center", va="bottom", fontsize=fs, color=color,
                bbox=dict(fc="white", ec="none", alpha=0.85, pad=0.5))


def lienzo(w, h, xr, yr):
    fig, ax = plt.subplots(figsize=(w, h))
    ax.set_xlim(*xr)
    ax.set_ylim(*yr)
    ax.axis("off")
    return fig, ax


# --------------------------------------------------------------------------
#  1. Diagrama de conexion
# --------------------------------------------------------------------------
def diagrama_conexion():
    fig, ax = lienzo(16, 10, (0, 16), (0, 10))
    ax.text(8, 9.75, "Redsiliente — diagrama de conexión", ha="center", fontsize=15,
            fontweight="bold")

    # ---------------- Nodo LILYGO ----------------
    titulo_caja(ax, 0.2, 0.3, 7.6, 9.0, "NODO EMISOR (x2) — LILYGO TTGO LoRa32 T3_V1.6.1", AZUL)
    caja(ax, 2.7, 4.3, 2.4, 2.4, "ESP32\n(ESP32-PICO-D4)\n240 MHz\nArduino-ESP32", "#c9dcf2",
         fs=9, bold=True)
    caja(ax, 5.6, 6.6, 2.0, 1.5, "SX1276\n915 MHz\n(interno)", NARANJA, fs=9, bold=True)
    caja(ax, 5.6, 4.0, 2.0, 1.2, "OLED SSD1306\n128x64  I2C 0x3C", VERDE, fs=8)
    caja(ax, 5.6, 1.0, 2.0, 1.3, "Batería LiPo\n(JST) + divisor\n100k/100k", GRIS, fs=8)
    caja(ax, 0.5, 6.7, 2.0, 1.4, "Antena 915 MHz\n(SMA/IPEX)\n¡conectar antes\nde energizar!",
         ROJO, fs=7.5)
    caja(ax, 2.7, 1.0, 2.4, 1.3, "USB ↔ UART0\n(GPIO1 TX / GPIO3 RX)\nmonitor 115200 Bd", GRIS, fs=8)
    caja(ax, 0.5, 2.6, 1.8, 3.4, "Teclado\nmatricial\n4x4\n\npines 1-8\n(izq→der)", MORADO, fs=8.5,
         bold=False)

    flecha(ax, (5.1, 6.2), (5.6, 7.2), "SPI\nSCK 5 · MISO 19\nMOSI 27 · NSS 18", both=True,
           off=(-1.0, 0.05), fs=7)
    flecha(ax, (6.6, 6.6), (4.9, 6.7), "", color="#b35c00")
    ax.text(6.1, 6.15, "RST 23 · DIO0 26 · DIO1 33", fontsize=7, color="#b35c00", ha="center")
    flecha(ax, (5.1, 4.9), (5.6, 4.6), "I2C SDA 21 / SCL 22", both=True, off=(0.4, -0.55), fs=7)
    flecha(ax, (6.6, 2.3), (4.6, 4.3), "ADC1 IO35\n(Vbat/2)", off=(0.6, -0.3), fs=7)
    flecha(ax, (3.9, 2.3), (3.9, 4.3), "", both=True)
    flecha(ax, (2.5, 7.8), (5.6, 7.8), "", both=True, color="#aa0000")
    ax.text(4.05, 7.9, "RF (ANT)", fontsize=7, color="#aa0000", ha="center")
    flecha(ax, (2.3, 4.8), (2.7, 5.4), "", both=True)
    ax.text(0.45, 1.65 + 0.0, "", fontsize=7)
    filas = "Filas (entrada pull-up):\n1→IO14  2→IO12\n3→IO13  4→IO15"
    cols = "Columnas (salida barrido):\n5→IO2   6→IO0\n7→IO4   8→IO25"
    ax.text(0.45, 2.45, filas + "\n" + cols, fontsize=7, va="top", family="monospace")
    ax.text(0.45, 0.95, "IO0, IO2, IO12, IO15 = pines de arranque: no pulsar al reset.\n"
                        "IO25 es también el LED de la placa. microSD vacía.", fontsize=7.2,
            va="top", color="#7a0000")

    # ---------------- Gateway ----------------
    titulo_caja(ax, 8.2, 0.3, 7.6, 9.0, "GATEWAY — ST B-L072Z-LRWAN1", VERDE)
    ax.add_patch(FancyBboxPatch((8.5, 3.6), 4.6, 4.6, boxstyle="round,pad=0.02",
                                fc="white", ec="#666666", lw=1.2, ls="--"))
    ax.text(8.6, 8.05, "Módulo Murata CMWX1ZZABZ-091", fontsize=8.5, fontweight="bold", va="top")
    caja(ax, 8.7, 4.0, 2.0, 2.6, "STM32L072CZ\nCortex-M0+\n32 MHz\n192 KB Flash\n20 KB RAM",
         "#c9dcf2", fs=8.5, bold=True)
    caja(ax, 11.0, 5.6, 1.9, 1.6, "SX1276", NARANJA, fs=9, bold=True)
    caja(ax, 11.0, 4.15, 1.9, 1.0, "TCXO 32 MHz\n(VCC = PA12)", GRIS, fs=7.5)
    flecha(ax, (10.7, 6.3), (11.0, 6.3), "", both=True)
    ax.text(10.85, 7.5, "SPI1: MOSI PA7 · MISO PA6\nSCK PB3 · NSS PA15\nRST PC0 · DIO0 PB4 · DIO1 PB1",
            fontsize=6.8, ha="center", va="center")
    flecha(ax, (11.95, 5.15), (11.95, 5.6), "")
    caja(ax, 13.4, 5.6, 2.2, 1.6, "Conmutador RF\nRX: PA1\nTX_BOOST: PC1\nTX_RFO: PC2", ROJO, fs=7.5)
    flecha(ax, (12.9, 6.4), (13.4, 6.4), "", both=True, color="#aa0000")
    caja(ax, 13.6, 7.8, 1.9, 1.0, "Antena 915 MHz\nSMA CN10", ROJO, fs=7.5)
    flecha(ax, (14.5, 7.2), (14.5, 7.8), "", both=True, color="#aa0000")
    caja(ax, 8.7, 1.0, 2.6, 1.8, "ST-LINK/V2-1\n(USB CN7)\nSWD: PA13/PA14\nVCP: USART2", GRIS, fs=8)
    flecha(ax, (9.7, 4.0), (9.7, 2.8), "USART2\nPA2 TX / PA3 RX\n115200 8N1", both=True,
           off=(0.0, -0.35), fs=7)
    caja(ax, 12.0, 1.0, 3.6, 1.8, "PC (PMU)\nbackend serial_bridge.py\n→ tablero offline", GRIS,
         fs=8.5)
    flecha(ax, (11.3, 1.9), (12.0, 1.9), "USB", both=True)
    caja(ax, 13.4, 3.4, 2.2, 1.6, "LEDs\nLD1 verde PB5\nLD3 azul PB6\nLD4 rojo PB7", VERDE, fs=7.5)
    flecha(ax, (10.7, 4.05), (13.4, 3.7), "GPIO", off=(0.9, -0.35), fs=7)
    ax.text(8.5, 0.55, "LD2 (PA5) no se usa: comparte pin con DIO4 del SX1276.", fontsize=7.2,
            color="#7a0000")

    # Enlace radio
    ax.annotate("", xy=(13.6, 8.6), xytext=(1.5, 8.1),
                arrowprops=dict(arrowstyle="<|-|>", color="#aa0000", lw=1.6, ls="--",
                                connectionstyle="arc3,rad=-0.04"))
    ax.text(7.9, 9.3, "LoRa P2P 915 MHz · BW 125 kHz · SF7 · CR 4/5 · SW 0x12",
            ha="center", fontsize=9, color="#aa0000",
            bbox=dict(fc="white", ec="#aa0000", pad=2))
    fig.savefig(IMG / "diagrama-conexion.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------
#  2. Diagrama de bloques
# --------------------------------------------------------------------------
def diagrama_bloques():
    fig, ax = lienzo(16, 7.5, (0, 16), (0, 7.5))
    ax.text(8, 7.2, "Redsiliente — diagrama de bloques y señales", ha="center", fontsize=15,
            fontweight="bold")

    titulo_caja(ax, 0.2, 0.4, 5.6, 6.3, "Nodo (ESP32)", AZUL)
    caja(ax, 0.5, 4.8, 2.0, 1.1, "Teclado 4x4\n(Keypad)", MORADO, fs=8.5)
    caja(ax, 0.5, 3.2, 2.0, 1.1, "ADC IO35\nbatería", GRIS, fs=8.5)
    caja(ax, 0.5, 1.5, 2.0, 1.2, "NVS (Preferences)\nSEQ persistente", GRIS, fs=8)
    caja(ax, 3.0, 2.9, 2.5, 2.4, "Lógica del nodo\n(máquina de estados)\n\nTrama + HMAC8\n(mbedtls)",
         "#c9dcf2", fs=8.5, bold=False)
    caja(ax, 3.0, 0.7, 2.5, 1.4, "OLED SSD1306\n(estado / enlace)", VERDE, fs=8.5)
    caja(ax, 3.0, 5.5, 2.5, 0.9, "SX1276 (RadioLib)", NARANJA, fs=8.5, bold=True)
    flecha(ax, (2.5, 5.35), (3.0, 4.7), "GPIO", fs=7, off=(0, 0.05))
    flecha(ax, (2.5, 3.75), (3.0, 3.75), "ADC", fs=7)
    flecha(ax, (2.5, 2.1), (3.0, 3.1), "", both=True)
    flecha(ax, (4.25, 2.9), (4.25, 2.1), "I2C", fs=7, off=(0.3, -0.1))
    flecha(ax, (4.25, 5.3), (4.25, 5.5), "", both=True)
    ax.text(5.55, 5.25, "SPI + DIO0", fontsize=7.5, ha="right", color="#1a4e8a")

    # aire
    caja(ax, 6.3, 3.9, 3.0, 1.6, "Aire — LoRa P2P\n915 MHz, SF7/125 kHz\ntrama (51 B) ≈ 103 ms\nACK ≈ 62 ms",
         ROJO, fs=8.5)
    flecha(ax, (5.5, 6.0), (6.6, 5.5), "DATOS (10 campos)", color="#aa0000", fs=7.5, off=(0.0, 0.1))
    flecha(ax, (9.3, 5.5), (10.4, 5.9), "", color="#aa0000")
    flecha(ax, (10.4, 5.55), (9.3, 4.4), "", color="#1a7a1a")
    flecha(ax, (6.6, 4.4), (5.5, 5.75), "ACK|ID|SEQ|HMAC8", color="#1a7a1a", fs=7.5, off=(-0.1, -0.7))

    titulo_caja(ax, 10.1, 0.4, 5.7, 6.3, "Gateway (STM32L072) y PMU", VERDE)
    caja(ax, 10.4, 5.3, 2.4, 0.85, "SX1276 (RadioLib)", NARANJA, fs=8.5, bold=True)
    caja(ax, 10.4, 3.3, 2.4, 1.6, "LoRaReceiver\nISR DIO0 → bandera\ncola de 4 paquetes", "#c9dcf2",
         fs=8)
    caja(ax, 13.1, 3.3, 2.5, 1.6, "MessageProcessor\nvalida 10 campos\nHMAC-SHA256 → ACK", "#c9dcf2",
         fs=8)
    caja(ax, 13.1, 5.3, 2.5, 0.85, "LEDs LD1/LD3/LD4", VERDE, fs=8)
    caja(ax, 10.4, 1.0, 2.4, 1.5, "USART2 → ST-LINK\nVCP 115200 Bd", GRIS, fs=8)
    caja(ax, 13.1, 1.0, 2.5, 1.5, "PC: backend\n(serial_bridge)\n→ tablero PMU", GRIS, fs=8)
    flecha(ax, (11.6, 5.3), (11.6, 4.9), "SPI/DIO0", fs=7, off=(0.55, -0.1), both=True)
    flecha(ax, (12.8, 4.1), (13.1, 4.1), "")
    flecha(ax, (14.35, 4.9), (14.35, 5.3), "GPIO", fs=7, off=(0.4, -0.1))
    flecha(ax, (13.1, 3.5), (12.4, 2.5), "trama|RSSI", fs=7, off=(0.5, -0.05))
    flecha(ax, (12.8, 1.75), (13.1, 1.75), "USB", fs=7)

    # puntos de medida
    for (x, y, n) in [(4.25, 5.4, "A"), (11.6, 5.2, "B"), (11.6, 2.75, "C")]:
        ax.add_patch(plt.Circle((x - 0.95, y), 0.17, color="#ffcc00", ec="black", zorder=5))
        ax.text(x - 0.95, y, n, ha="center", va="center", fontsize=8, fontweight="bold", zorder=6)
    ax.text(8, 0.05, "Puntos de medida Logic 2: (A) SPI+DIO0 del nodo en el header de la LILYGO; "
                     "(B) DIO0 del gateway; (C) USART2 TX (PA2) hacia el PC.",
            ha="center", fontsize=8.5)
    fig.savefig(IMG / "diagrama-bloques.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------
#  3. Maquinas de estados
# --------------------------------------------------------------------------
def estado(ax, x, y, texto, color="#c9dcf2", w=2.3, h=0.9, fs=8):
    caja(ax, x - w / 2, y - h / 2, w, h, texto, color, fs=fs, bold=False)
    return (x, y, w, h)


def maquina_estados():
    fig, ax = lienzo(16, 13, (0, 16), (0, 13))
    ax.text(8, 12.75, "Máquinas de estados", ha="center", fontsize=15, fontweight="bold")

    # ---------------- Nodo ----------------
    titulo_caja(ax, 0.2, 5.4, 15.6, 7.0, "Nodo emisor (node_reporter/src/main.cpp)", AZUL)
    s_ini = estado(ax, 1.7, 10.9, "INICIO\nOLED, NVS(SEQ),\nradio.begin()", GRIS)
    s_rep = estado(ax, 5.2, 10.9, "REPOSO\npantalla de enlace\n(cada 1 s)", VERDE, h=1.0)
    s_ev = estado(ax, 9.3, 10.9, "EVENTO ELEGIDO\n(pendiente de #)")
    s_qty = estado(ax, 13.6, 10.9, "CAPTURA CANTIDAD\n(solo brigadista)\n1-2 dígitos", h=1.0)
    s_tx = estado(ax, 9.3, 8.6, "TRANSMITIR\nSEQ++ → NVS, HMAC8\nradio.transmit()", NARANJA, h=1.0)
    s_ack = estado(ax, 13.6, 8.3, "ESPERA ACK\nRX 1,5 s (DIO0)", NARANJA)
    s_rt = estado(ax, 13.6, 6.2, "PAUSA\naleatoria 0,4-1,2 s", GRIS)
    s_ok = estado(ax, 5.2, 8.3, "CONFIRMADO\n\"RECIBIDO POR PMU\"", VERDE)
    s_nok = estado(ax, 5.2, 6.2, "SIN CONFIRMACIÓN\n(enlace: NO RESPONDE)", ROJO)
    s_err = estado(ax, 1.7, 8.3, "ERROR LORA\n(bloqueado)", ROJO, w=2.0)

    flecha(ax, (2.85, 10.9), (4.05, 10.9), "begin OK", fs=7)
    flecha(ax, (1.7, 10.45), (1.7, 8.75), "begin ≠ OK", fs=7, off=(0.55, 0))
    flecha(ax, (6.35, 11.05), (8.15, 11.05), "tecla 0-9 permitida", fs=7)
    flecha(ax, (8.15, 10.75), (6.35, 10.75), "* (vacío) / 15 s sin tecla", fs=7, off=(0, -0.42))
    flecha(ax, (10.45, 11.05), (12.45, 11.05), "brigadista y evento\ncon cantidad", fs=7)
    flecha(ax, (13.6, 11.4), (13.6, 11.9), "", fs=7)
    ax.text(13.6, 12.0, "dígito / * borra", fontsize=7, ha="center")
    flecha(ax, (9.3, 10.45), (9.3, 9.1), "#", fs=8, off=(0.2, 0))
    flecha(ax, (13.1, 10.4), (10.2, 9.1), "#", fs=8, off=(0.25, 0.1))
    flecha(ax, (5.2, 10.4), (8.15, 8.9), "latido (cada 5 min\n+ jitter ≤10 s) o 'h'", fs=7,
           off=(-0.6, -0.1))
    flecha(ax, (10.45, 8.6), (12.45, 8.6), "TxDone", fs=7)
    flecha(ax, (12.45, 7.95), (6.35, 7.95), "ACK válido (ID, SEQ, HMAC)", fs=7, off=(0, -0.42),
           color="#1a7a1a")
    flecha(ax, (13.6, 7.85), (13.6, 6.65), "timeout,\nintento < 3", fs=7, off=(0.7, -0.1))
    flecha(ax, (12.45, 6.2), (9.6, 8.1), "misma trama\n(mismo SEQ)", fs=7, off=(0.6, -0.85))
    flecha(ax, (12.9, 7.85), (6.35, 6.4), "timeout, intento = 3", fs=7, off=(-1.5, -0.7),
           color="#aa0000")
    flecha(ax, (4.3, 8.75), (4.6, 10.4), "4 s", fs=7, off=(-0.35, 0))
    flecha(ax, (4.0, 6.4), (4.0, 10.4), "4 s", fs=7, off=(-0.35, 0))
    ax.text(0.45, 5.65, "Estados del enlace en REPOSO: sin datos → OK (ACK reciente) → NO RESPONDE "
                        "(último envío sin ACK) → SIN ENLACE (último ACK > 11 min).", fontsize=8)

    # ---------------- Gateway ----------------
    titulo_caja(ax, 0.2, 0.2, 15.6, 5.0, "Gateway (gateway_receiver: LoRaReceiver + MessageProcessor)",
                VERDE)
    g_tcxo = estado(ax, 1.8, 3.8, "POWER_UP_TCXO\nPA12 = 1, esperar 5 ms", GRIS, w=2.5)
    g_init = estado(ax, 5.3, 3.8, "INIT_RADIO\nbegin(), RegTcxo,\nCRC on, ISR DIO0", w=2.5, h=1.0)
    g_start = estado(ax, 8.8, 3.8, "START_RX\nstartReceive()", w=2.3)
    g_lis = estado(ax, 12.6, 3.8, "LISTENING\nLD3 1 Hz", VERDE, w=2.3)
    g_err = estado(ax, 5.3, 1.3, "ERROR_BACKOFF\n2 s, LD3 5 Hz", ROJO, w=2.5)
    g_proc = estado(ax, 12.6, 1.3, "LEER FIFO → cola\nvalidar, reenviar USB,\nfirma OK → TX ACK",
                    NARANJA, w=3.0, h=1.1)

    flecha(ax, (3.05, 3.8), (4.05, 3.8), "≥ 5 ms", fs=7)
    flecha(ax, (6.55, 3.8), (7.65, 3.8), "OK", fs=7)
    flecha(ax, (9.95, 3.8), (11.45, 3.8), "OK", fs=7)
    flecha(ax, (12.6, 3.35), (12.6, 1.85), "DIO0 (RxDone)\nbandera ISR", fs=7, off=(0.95, -0.1))
    flecha(ax, (11.1, 1.5), (9.1, 3.35), "re-armar RX", fs=7, off=(-0.6, -0.2))
    flecha(ax, (5.3, 3.3), (5.3, 1.75), "begin ≠ OK", fs=7, off=(0.6, 0))
    flecha(ax, (8.8, 3.35), (6.55, 1.5), "startReceive ≠ OK", fs=7, off=(0.5, -0.35))
    flecha(ax, (4.05, 1.3), (4.4, 3.3), "2 s", fs=7, off=(-0.4, -0.1))
    fig.savefig(IMG / "maquina-estados.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------
#  4. Temporizacion esperada (referencia para la captura de Logic 2)
# --------------------------------------------------------------------------
def temporizacion(toa_trama, toa_ack, uart_ms):
    fig, ax = plt.subplots(figsize=(16, 6.5))
    canales = ["Nodo NSS (IO18)", "Nodo SCK (IO5)", "Nodo DIO0 (IO26)", "Aire (RF)",
               "GW DIO0 (PB4)", "GW USART2 TX (PA2)"]
    n = len(canales)

    t_cfg = 1.0                 # configuracion + escritura FIFO por SPI
    t_tx0 = t_cfg
    t_tx1 = t_tx0 + toa_trama   # fin de trama en el aire
    t_rd = t_tx1 + 0.6          # lectura del FIFO en el gateway
    t_u0 = t_rd + 0.2
    t_u1 = t_u0 + uart_ms
    t_ack0 = t_u1 + 1.5         # HMAC + configuracion TX
    t_ack1 = t_ack0 + toa_ack
    t_end = t_ack1 + 15

    def nivel(i, segs, alto=0.6):
        y0 = (n - 1 - i) * 1.0
        xs, ys = [0], [y0]
        for (a, b) in segs:
            xs += [a, a, b, b]
            ys += [y0, y0 + alto, y0 + alto, y0]
        xs.append(t_end)
        ys.append(y0)
        return xs, ys

    def rafaga(i, a, b, texto):
        y0 = (n - 1 - i) * 1.0
        ax.add_patch(plt.Rectangle((a, y0), max(b - a, 0.8), 0.6, fc="#4f81bd", ec="none"))
        ax.text(b + 1, y0 + 0.3, texto, fontsize=7.5, va="center")

    # NSS activo bajo: alto en reposo, bajos durante las rafagas SPI
    y = (n - 1) * 1.0
    xs = [0, 0.1, 0.1, t_cfg, t_cfg, t_tx1 + 0.1, t_tx1 + 0.1, t_tx1 + 0.5, t_tx1 + 0.5,
          t_ack1 + 0.2, t_ack1 + 0.2, t_ack1 + 0.6, t_ack1 + 0.6, t_end]
    ys = [y + .6, y + .6, y, y, y + .6, y + .6, y, y, y + .6, y + .6, y, y, y + .6, y + .6]
    ax.plot(xs, ys, color="black", lw=1.2)
    rafaga(1, 0.1, t_cfg, "config + FIFO (≈66 B @ 2 MHz ≈ 0,3 ms)")
    rafaga(1, t_tx1 + 0.1, t_tx1 + 0.5, "IRQ clear + RX")
    rafaga(1, t_ack1 + 0.2, t_ack1 + 0.6, "leer ACK")
    xs, ys = nivel(2, [(t_tx1, t_tx1 + 0.3), (t_ack1, t_ack1 + 0.3)])
    ax.plot(xs, ys, color="black", lw=1.2)
    ax.text(t_tx1 + 1, (n - 3) + 0.65, "TxDone", fontsize=7.5)
    ax.text(t_ack1 + 1, (n - 3) + 0.65, "RxDone (ACK)", fontsize=7.5)
    y = (n - 4) * 1.0
    ax.add_patch(plt.Rectangle((t_tx0, y), toa_trama, 0.6, fc="#f4b183", ec="black"))
    ax.text(t_tx0 + toa_trama / 2, y + 0.3, f"trama datos ≈ {toa_trama:.1f} ms", ha="center",
            fontsize=8, va="center")
    ax.add_patch(plt.Rectangle((t_ack0, y), toa_ack, 0.6, fc="#a9d18e", ec="black"))
    ax.text(t_ack0 + toa_ack / 2, y + 0.3, f"ACK ≈ {toa_ack:.1f} ms", ha="center", fontsize=8,
            va="center")
    xs, ys = nivel(4, [(t_tx1, t_rd), (t_ack1, t_ack1 + 0.3)])
    ax.plot(xs, ys, color="black", lw=1.2)
    ax.text(t_tx1 + 1, (n - 5) + 0.65, "RxDone", fontsize=7.5)
    ax.text(t_ack1 + 1, (n - 5) + 0.65, "TxDone", fontsize=7.5)
    y = (n - 6) * 1.0
    ax.plot([0, t_u0], [y + .6, y + .6], color="black", lw=1.2)
    ax.add_patch(plt.Rectangle((t_u0, y), uart_ms, 0.6, fc="#4f81bd", ec="none"))
    ax.plot([t_u1, t_end], [y + .6, y + .6], color="black", lw=1.2)
    ax.text(t_u1 + 1, y + 0.3, f"\"NODE-02|…|HMAC8|RSSI\\r\\n\" ≈ {uart_ms:.1f} ms @115200",
            fontsize=7.5, va="center")

    ax.set_yticks([(n - 1 - i) + 0.3 for i in range(n)])
    ax.set_yticklabels(canales, fontsize=9)
    ax.set_xlim(0, t_end)
    ax.set_ylim(-0.3, n)
    ax.set_xlabel("tiempo [ms] (t = 0: el nodo baja NSS para configurar la transmisión)")
    ax.grid(axis="x", ls=":", alpha=0.6)
    for s in ("top", "right", "left"):
        ax.spines[s].set_visible(False)
    ax.set_title("Secuencia esperada de un reporte con ACK (calculada; referencia para la captura "
                 "de Logic 2)", fontsize=12, fontweight="bold")
    fig.savefig(IMG / "logic2-esperado.png", dpi=150, bbox_inches="tight")
    plt.close(fig)


if __name__ == "__main__":
    trama = "NODE-02|14|2|5.0714|-75.5246|0|3920|0.02|3|60da5124"
    ack = "ACK|NODE-02|14|b8905636"
    toa_t, tsym, nsym = lora_toa_ms(len(trama))
    toa_a, _, nsym_a = lora_toa_ms(len(ack))
    uart = (len(trama) + len("|-84") + 2) * 10 / 115200 * 1000
    print(f"Tsym = {tsym:.3f} ms")
    print(f"Trama {len(trama)} B: {nsym} simbolos de payload, ToA = {toa_t:.1f} ms")
    print(f"ACK   {len(ack)} B: {nsym_a} simbolos de payload, ToA = {toa_a:.1f} ms")
    print(f"UART  {len(trama) + 6} caracteres a 115200 8N1 = {uart:.2f} ms")
    for pl in (40, 52, 65, 80, 255):
        print(f"  payload {pl:3d} B -> {lora_toa_ms(pl)[0]:.1f} ms")
    diagrama_conexion()
    diagrama_bloques()
    maquina_estados()
    temporizacion(toa_t, toa_a, uart)
    print("Figuras en", IMG)
