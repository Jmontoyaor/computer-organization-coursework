#ifndef UART_H
#define UART_H

/* Inicializacion (vacia: la UART del SoC no necesita configuracion por software;
 * 115200 baudios 8N1 vienen fijados por hardware en hw/modules/CPU_UART.v). */
void uart_init(void);

/* Envia un byte; espera (polling) mientras el transmisor esta ocupado. */
void uart_putchar(char c);

/* Envia una cadena terminada en '\0'. */
void uart_putstr(const char* str);

/* Recepcion NO bloqueante: devuelve el byte recibido (0..255) o -1 si no hay
 * ningun dato pendiente. Leer el byte limpia la bandera rx_ready en hardware. */
int uart_getchar_nb(void);

#endif
