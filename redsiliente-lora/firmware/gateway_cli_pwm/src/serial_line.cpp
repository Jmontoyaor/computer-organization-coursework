// =============================================================================
//  serial_line.cpp - Ensamblado de lineas con eco y borrado
// =============================================================================
#include "serial_line.h"
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

namespace {

constexpr char ASCII_BACKSPACE = 0x08;
constexpr char ASCII_DELETE = 0x7F;
constexpr size_t PRINTF_BUFFER = 160;

char   g_line[SERIAL_LINE_MAX + 1];
size_t g_length = 0;
bool   g_overflowed = false;  // se descarta hasta el siguiente terminador
bool   g_last_was_cr = false;

}  // namespace

void serial_init(uint32_t baud) {
    Serial.begin(baud);  // STM32duino: Serial = USART2, 8N1
}

SerialLineResult serial_task(const char** line) {
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());

        if (c == '\n' && g_last_was_cr) {  // el '\n' de un "\r\n" ya se proceso
            g_last_was_cr = false;
            continue;
        }
        g_last_was_cr = (c == '\r');

        if (c == '\r' || c == '\n') {
            Serial.print("\r\n");
            const bool overflowed = g_overflowed;
            g_line[g_length] = '\0';
            g_length = 0;
            g_overflowed = false;
            if (overflowed) return SERIAL_LINE_OVERFLOW;
            *line = g_line;
            return SERIAL_LINE_READY;
        }
        if (c == ASCII_BACKSPACE || c == ASCII_DELETE) {
            if (g_length > 0) {
                g_length--;
                Serial.print("\b \b");
            }
            continue;
        }
        if (c < 0x20 || c > 0x7E) continue;  // ignora control y no ASCII

        if (g_length >= SERIAL_LINE_MAX) {
            g_overflowed = true;  // no se escribe fuera del buffer
            continue;
        }
        g_line[g_length++] = c;
        Serial.write(c);  // eco: el monitor no muestra lo que se teclea
    }
    return SERIAL_LINE_NONE;
}

void serial_printf(const char* format, ...) {
    char buffer[PRINTF_BUFFER];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
}
