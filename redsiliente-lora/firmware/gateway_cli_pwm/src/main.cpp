// =============================================================================
//  main.cpp - Gateway Redsiliente: reloj, PWM, boton, CLI y perifericos
// -----------------------------------------------------------------------------
//  El reloj lo configura SystemClock_Config() (system_clock.cpp) antes de
//  setup(). loop() es cooperativo: cada tarea revisa su estado y vuelve en
//  microsegundos, sin delay().
//
//    button_task()      -> antirrebote y evento de pulsacion
//    serial_task()      -> junta caracteres hasta tener una linea
//    cli_process_line() -> interpreta y valida la orden
//    application_task() -> avanza el fade y actualiza el duty
// =============================================================================
#include <Arduino.h>
#include "adc_monitor.h"
#include "application.h"
#include "board_pins.h"
#include "button.h"
#include "cli.h"
#include "pwm.h"
#include "radio_probe.h"
#include "serial_line.h"

#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif
#ifndef BUTTON_INTERNAL_PULLUP
#define BUTTON_INTERNAL_PULLUP 0
#endif

namespace {
constexpr uint32_t PWM_DEFAULT_FREQUENCY_HZ = 1000;  // sin parpadeo visible
}  // namespace

void setup() {
    serial_init(SERIAL_BAUD);
    if (!pwm_init(BoardPins::PWM_LED, PWM_DEFAULT_FREQUENCY_HZ, 0)) {
        serial_printf("ERROR: PWM pin has no timer channel\r\n");
    }
    button_init(BoardPins::USER_BUTTON, BUTTON_INTERNAL_PULLUP);
    adc_monitor_init();
    radio_probe_init();
    application_init();
    cli_init();
    cli_print_prompt();
}

void loop() {
    const uint32_t now_ms = millis();

    if (button_task(now_ms)) {
        application_on_button_press();
    }

    const char* line = nullptr;
    switch (serial_task(&line)) {
        case SERIAL_LINE_READY:
            cli_process_line(line);
            cli_print_prompt();
            break;
        case SERIAL_LINE_OVERFLOW:
            cli_report_overflow();
            cli_print_prompt();
            break;
        case SERIAL_LINE_NONE:
            break;
    }

    application_task(now_ms);
}
