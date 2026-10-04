/*
 * gpio.c - Driver del registro de LEDs del SoC.
 *
 * El SoC (hw/SOC.v) decodifica la direccion 0x00010000 como un registro de
 * 4 bits (led_reg). Solo se guardan los 4 bits bajos del dato escrito; el
 * hardware invierte la salida porque los LEDs de la Tang Primer 20K son
 * activos en bajo. Desde el software, por tanto: 1 = LED encendido.
 */
#include "gpio.h"

/* Registro de LEDs mapeado en memoria (bits 3..0 = LED3..LED0). */
#define LED_REG (*(volatile unsigned int*)0x00010000)

/* Deja todos los LEDs apagados. */
void gpio_init(void) {

    LED_REG = 0;
}

/* Escribe el patron de LEDs: el bit i de `value` gobierna el LED i. */
void gpio_write_leds(unsigned int value) {
    LED_REG = value;
}
