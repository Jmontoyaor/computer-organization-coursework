/*
 * uart.c - Driver de la UART del SoC (hw/modules/CPU_UART.v), por polling.
 *
 * Mapa de registros (base 0x00020000):
 *   +0x0  DATOS   escritura: transmite el byte bajo   lectura: ultimo byte recibido
 *   +0x4  ESTADO  bit 0 = tx_busy (1: transmisor ocupado, no escribir)
 *                 bit 1 = rx_ready (1: hay un byte recibido sin leer)
 */
#include "uart.h"

#define UART_DATA   (*(volatile unsigned int*)0x00020000)
#define UART_STATUS (*(volatile unsigned int*)0x00020004)

#define UART_TX_BUSY  (1u << 0)   /* transmisor ocupado */
#define UART_RX_READY (1u << 1)   /* dato recibido disponible */

void uart_init(void) {

}

void uart_putchar(char c) {
    while (UART_STATUS & UART_TX_BUSY);   /* espera a que el transmisor quede libre */
    UART_DATA = c;                        /* dispara la transmision del byte */
}

void uart_putstr(const char* str) {
    while (*str) {
        uart_putchar(*str++);
    }
}

int uart_getchar_nb(void) {
    if (!(UART_STATUS & UART_RX_READY)) return -1;   /* nada pendiente */
    return (int)(UART_DATA & 0xFF);                  /* la lectura limpia rx_ready */
}
