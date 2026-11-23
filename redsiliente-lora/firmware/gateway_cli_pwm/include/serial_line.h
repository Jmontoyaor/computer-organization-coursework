// =============================================================================
//  serial_line.h - Recepcion no bloqueante de lineas por la UART
// -----------------------------------------------------------------------------
//  USART2 (PA2 TX / PA3 RX) va al ST-LINK y aparece en el PC como puerto COM
//  virtual. Formato 115200 baudios, 8 bits de datos, sin paridad, 1 bit de parada.
//  Una linea termina en '\r', '\n' o "\r\n". Este modulo solo junta caracteres:
//  no sabe nada de comandos.
// =============================================================================
#pragma once
#include <stddef.h>
#include <stdint.h>

constexpr size_t SERIAL_LINE_MAX = 64;  // caracteres utiles por linea

enum SerialLineResult : uint8_t {
    SERIAL_LINE_NONE,      // aun no hay linea completa
    SERIAL_LINE_READY,     // hay una linea en el buffer
    SERIAL_LINE_OVERFLOW,  // la linea supero SERIAL_LINE_MAX y se descarto
};

void serial_init(uint32_t baud);

// Procesa lo que haya llegado. Con SERIAL_LINE_READY, *line apunta a la linea
// (terminada en '\0'), valida hasta la siguiente llamada.
SerialLineResult serial_task(const char** line);

// Salida con formato (usa vsnprintf; sin float).
void serial_printf(const char* format, ...) __attribute__((format(printf, 1, 2)));
