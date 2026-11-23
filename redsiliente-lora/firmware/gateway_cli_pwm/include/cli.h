// =============================================================================
//  cli.h - Interprete de comandos
// -----------------------------------------------------------------------------
//  Recibe una linea ya completa (serial_line), la parte en palabras, busca el
//  comando en una tabla, valida los argumentos y llama al modulo que corresponde.
//  Ningun comando bloquea mas de unos milisegundos.
// =============================================================================
#pragma once

void cli_init();
void cli_print_prompt();
void cli_process_line(const char* line);
void cli_report_overflow();
