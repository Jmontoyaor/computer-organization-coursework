#ifndef GPIO_H
#define GPIO_H

/* Apaga todos los LEDs. Llamar una vez al arrancar. */
void gpio_init(void);

/* Escribe el patron de LEDs (bit i = LED i, 1 = encendido; solo 4 bits utiles). */
void gpio_write_leds(unsigned int value);

#endif
