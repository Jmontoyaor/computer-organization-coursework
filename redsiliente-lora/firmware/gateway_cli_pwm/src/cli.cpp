// =============================================================================
//  cli.cpp - Tabla de comandos, validacion y respuestas
// =============================================================================
#include "cli.h"
#include <stdlib.h>
#include <string.h>
#include "adc_monitor.h"
#include "application.h"
#include "button.h"
#include "pwm.h"
#include "radio_probe.h"
#include "serial_line.h"
#include "system_clock.h"

namespace {

constexpr size_t CLI_MAX_ARGS = 4;

struct CliCommand {
    const char* name;
    const char* usage;
    const char* help;
    void (*handler)(int argc, char* argv[]);
};

// ---- utilidades -------------------------------------------------------------

// Entero decimal sin signo; rechaza vacio, signos, letras y desbordes.
bool parse_uint(const char* text, uint32_t* value) {
    if (text == nullptr || *text == '\0' || strlen(text) > 9) return false;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    *value = static_cast<uint32_t>(strtoul(text, nullptr, 10));
    return true;
}

void print_permille(uint32_t permille) {
    serial_printf("%lu.%lu", static_cast<unsigned long>(permille / 10),
                  static_cast<unsigned long>(permille % 10));
}

void print_millihertz(uint32_t mhz) {
    serial_printf("%lu.%03lu", static_cast<unsigned long>(mhz / 1000),
                  static_cast<unsigned long>(mhz % 1000));
}

void error(const char* message) { serial_printf("ERROR: %s\r\n", message); }

// ---- comandos ---------------------------------------------------------------

void cmd_help(int, char**);

void cmd_status(int, char**) {
    ClockStatus clk;
    PwmStatus pwm;
    AppStatus app;
    ButtonStatus btn;
    system_clock_get_status(&clk);
    pwm_get_status(&pwm);
    application_get_status(&app);
    button_get_status(&btn);

    serial_printf("System clock: %lu Hz (%s)\r\n", static_cast<unsigned long>(clk.sysclk_hz),
                  clk.sysclk_source);
    serial_printf("Timer clock: %lu Hz (%s CH%lu)\r\n",
                  static_cast<unsigned long>(pwm.timer_clock_hz), pwm.timer_name,
                  static_cast<unsigned long>(pwm.channel));
    serial_printf("PWM frequency: ");
    print_millihertz(pwm.actual_frequency_mhz);
    serial_printf(" Hz (requested %lu Hz)\r\n",
                  static_cast<unsigned long>(pwm.requested_frequency_hz));
    serial_printf("Duty cycle: ");
    print_permille(pwm.actual_duty_permille);
    serial_printf(" %% (PSC=%lu ARR=%lu CCR=%lu)\r\n", static_cast<unsigned long>(pwm.psc),
                  static_cast<unsigned long>(pwm.arr), static_cast<unsigned long>(pwm.ccr));
    serial_printf("PWM output: %s\r\n", pwm.output_enabled ? "enabled" : "disabled");
    serial_printf("Mode: %s, curve: %s, brightness: %lu %%\r\n", application_mode_name(app.mode),
                  brightness_curve_name(app.curve),
                  static_cast<unsigned long>(app.brightness_percent));
    serial_printf("Button: %s, presses: %lu\r\n", button_state_name(btn.state),
                  static_cast<unsigned long>(btn.press_events));
}

void cmd_frequency(int, char**) {
    PwmStatus pwm;
    pwm_get_status(&pwm);
    serial_printf("PWM frequency: ");
    print_millihertz(pwm.actual_frequency_mhz);
    serial_printf(" Hz = %lu / ((%lu + 1) * (%lu + 1))\r\n",
                  static_cast<unsigned long>(pwm.timer_clock_hz),
                  static_cast<unsigned long>(pwm.psc), static_cast<unsigned long>(pwm.arr));
}

void cmd_duty(int, char**) {
    PwmStatus pwm;
    pwm_get_status(&pwm);
    serial_printf("Duty cycle: ");
    print_permille(pwm.actual_duty_permille);
    serial_printf(" %% = %lu / (%lu + 1)\r\n", static_cast<unsigned long>(pwm.ccr),
                  static_cast<unsigned long>(pwm.arr));
}

void set_frequency(const char* arg) {
    uint32_t hz;
    if (!parse_uint(arg, &hz) || hz < PWM_MIN_FREQUENCY_HZ || hz > PWM_MAX_FREQUENCY_HZ) {
        serial_printf("ERROR: frequency must be an integer between %lu and %lu Hz\r\n",
                      static_cast<unsigned long>(PWM_MIN_FREQUENCY_HZ),
                      static_cast<unsigned long>(PWM_MAX_FREQUENCY_HZ));
        return;
    }
    if (!pwm_set_frequency_hz(hz)) {
        error("the timer cannot generate that frequency");
        return;
    }
    serial_printf("OK, applied ");
    cmd_frequency(0, nullptr);  // muestra el valor real, puede diferir del pedido
}

void set_duty(const char* arg) {
    uint32_t percent;
    if (!parse_uint(arg, &percent) || percent > 100) {
        error("duty must be an integer between 0 and 100 %");
        return;
    }
    application_set_duty_percent(percent);
    serial_printf("OK, applied ");
    cmd_duty(0, nullptr);
}

void set_brightness(const char* arg) {
    uint32_t percent;
    if (!parse_uint(arg, &percent) || !application_set_brightness(percent)) {
        error("brightness must be one of 0, 20, 40, 60, 80, 100");
        return;
    }
    serial_printf("OK, applied ");
    cmd_duty(0, nullptr);
}

bool parse_curve(const char* arg, BrightnessCurve* curve) {
    if (arg == nullptr) return false;
    if (strcmp(arg, "linear") == 0) *curve = CURVE_LINEAR;
    else if (strcmp(arg, "gamma") == 0) *curve = CURVE_GAMMA;
    else return false;
    return true;
}

void cmd_set(int argc, char* argv[]) {
    if (argc != 3) {
        error("usage: set <frequency|duty|brightness|curve> <value>");
        return;
    }
    const char* parameter = argv[1];
    const char* value = argv[2];
    if (strcmp(parameter, "frequency") == 0) {
        set_frequency(value);
    } else if (strcmp(parameter, "duty") == 0) {
        set_duty(value);
    } else if (strcmp(parameter, "brightness") == 0) {
        set_brightness(value);
    } else if (strcmp(parameter, "curve") == 0) {
        BrightnessCurve curve;
        if (!parse_curve(value, &curve)) {
            error("curve must be 'linear' or 'gamma'");
            return;
        }
        application_set_curve(curve);
        serial_printf("OK, curve: %s\r\n", brightness_curve_name(curve));
    } else {
        serial_printf("ERROR: unknown parameter '%s'\r\n", parameter);
    }
}

void cmd_fade(int argc, char* argv[]) {
    BrightnessCurve curve;
    if (argc == 2 && strcmp(argv[1], "off") == 0) {
        application_set_brightness(0);
        serial_printf("OK, fade stopped\r\n");
    } else if (argc == 2 && parse_curve(argv[1], &curve)) {
        application_start_fade(curve);
        serial_printf("OK, fading with %s curve (4 s per cycle)\r\n",
                      brightness_curve_name(curve));
    } else {
        error("usage: fade <linear|gamma|off>");
    }
}

bool parse_on_off(int argc, char* argv[], bool* on) {
    if (argc != 2) return false;
    if (strcmp(argv[1], "on") == 0) *on = true;
    else if (strcmp(argv[1], "off") == 0) *on = false;
    else return false;
    return true;
}

void cmd_pwm(int argc, char* argv[]) {
    bool on;
    if (!parse_on_off(argc, argv, &on)) {
        error("usage: pwm <on|off>");
        return;
    }
    pwm_set_output_enabled(on);
    serial_printf("OK, PWM output: %s\r\n", on ? "enabled" : "disabled");
}

void cmd_clock(int, char**) {
    ClockStatus c;
    system_clock_get_status(&c);
    serial_printf("SYSCLK source: %s", c.sysclk_source);
    if (strcmp(c.sysclk_source, "PLL") == 0) {
        serial_printf(" (%s x%lu /%lu)", c.pll_source, static_cast<unsigned long>(c.pll_mul),
                      static_cast<unsigned long>(c.pll_div));
    }
    serial_printf("\r\nSYSCLK: %lu Hz (from RCC registers), HAL: %lu Hz\r\n",
                  static_cast<unsigned long>(c.sysclk_hz),
                  static_cast<unsigned long>(c.hal_sysclk_hz));
    serial_printf("HCLK:  %lu Hz (AHB /%lu)\r\n", static_cast<unsigned long>(c.hclk_hz),
                  static_cast<unsigned long>(c.ahb_div));
    serial_printf("PCLK1: %lu Hz (APB1 /%lu)\r\n", static_cast<unsigned long>(c.pclk1_hz),
                  static_cast<unsigned long>(c.apb1_div));
    serial_printf("PCLK2: %lu Hz (APB2 /%lu)\r\n", static_cast<unsigned long>(c.pclk2_hz),
                  static_cast<unsigned long>(c.apb2_div));
    PwmStatus pwm;
    pwm_get_status(&pwm);
    serial_printf("APB1 timers (TIM2/3): %lu Hz, APB2 timers (TIM21/22): %lu Hz\r\n",
                  static_cast<unsigned long>(c.apb1_timer_hz),
                  static_cast<unsigned long>(c.apb2_timer_hz));
    serial_printf("PWM uses %s, HardwareTimer reports %lu Hz\r\n", pwm.timer_name,
                  static_cast<unsigned long>(pwm.timer_clock_hz));
    serial_printf("Serial = LPUART1 (PA2/PA3), clock %s, BRR=%lu -> %lu baud\r\n",
                  c.lpuart_source, static_cast<unsigned long>(c.lpuart_brr),
                  static_cast<unsigned long>(c.lpuart_baud));
    serial_printf("Flash latency: %lu WS, voltage range: %lu\r\n",
                  static_cast<unsigned long>(c.flash_latency),
                  static_cast<unsigned long>(c.voltage_range));
    serial_printf("MCO (PA8): %s", c.mco_enabled ? "SYSCLK/" : "off");
    if (c.mco_enabled) {
        serial_printf("%lu = %lu Hz", static_cast<unsigned long>(MCO_DIVIDER),
                      static_cast<unsigned long>(c.sysclk_hz / MCO_DIVIDER));
    }
    serial_printf("\r\n");
}

void cmd_mco(int argc, char* argv[]) {
    bool on;
    if (!parse_on_off(argc, argv, &on)) {
        error("usage: mco <on|off>");
        return;
    }
    system_clock_set_mco(on);
    cmd_clock(0, nullptr);
}

void cmd_timer(int, char**) {
    PwmStatus p;
    pwm_get_status(&p);
    serial_printf("%s channel %lu, input clock %lu Hz\r\n", p.timer_name,
                  static_cast<unsigned long>(p.channel),
                  static_cast<unsigned long>(p.timer_clock_hz));
    serial_printf("Counter: %s, %s\r\n", p.up_counting ? "up" : "down",
                  p.edge_aligned ? "edge-aligned" : "center-aligned");
    serial_printf("Output compare mode: %lu (%s), CCR preload: %s\r\n",
                  static_cast<unsigned long>(p.oc_mode),
                  p.oc_mode == 6 ? "PWM mode 1" : (p.oc_mode == 4 ? "forced inactive" : "other"),
                  p.preload_enabled ? "on" : "off");
    serial_printf("Polarity: active %s\r\n", p.active_high ? "high" : "low");
    serial_printf("PSC=%lu ARR=%lu CCR=%lu\r\n", static_cast<unsigned long>(p.psc),
                  static_cast<unsigned long>(p.arr), static_cast<unsigned long>(p.ccr));
}

void cmd_button(int, char**) {
    ButtonStatus b;
    button_get_status(&b);
    serial_printf("State: %s, pin now: %s, active level: %s\r\n", button_state_name(b.state),
                  b.raw_level ? "HIGH" : "LOW", b.active_high ? "HIGH" : "LOW");
    serial_printf("Raw edges: %lu, valid presses: %lu, bounces rejected: %lu\r\n",
                  static_cast<unsigned long>(b.raw_edges),
                  static_cast<unsigned long>(b.press_events),
                  static_cast<unsigned long>(b.rejected_bounces));
}

void cmd_adc(int, char**) {
    AdcReading r;
    adc_monitor_read(&r);
    if (r.vrefint_raw == 0) {
        // Sin VREFINT no se puede calcular VDDA ni corregir la temperatura.
        error("VREFINT reads 0: ADC not converting");
        return;
    }
    serial_printf("VREFINT raw: %lu (factory cal %lu at 3000 mV)\r\n",
                  static_cast<unsigned long>(r.vrefint_raw),
                  static_cast<unsigned long>(r.vrefint_cal));
    serial_printf("VDDA: %lu mV\r\n", static_cast<unsigned long>(r.vdda_mv));
    serial_printf("Temperature: %ld C (raw %lu)\r\n", static_cast<long>(r.temperature_c),
                  static_cast<unsigned long>(r.temperature_raw));
}

void cmd_radio(int, char**) {
    RadioProbeResult r;
    radio_probe_read(&r);
    serial_printf("SX1276 RegVersion (0x42): 0x%02X %s\r\n", r.version,
                  r.version_ok ? "(OK, expected 0x12)" : "(UNEXPECTED, expected 0x12)");
    serial_printf("RegOpMode (0x01): 0x%02X\r\n", r.op_mode);
    serial_printf("RegFrf (0x06-0x08): 0x%06lX = %lu kHz (reset value)\r\n",
                  static_cast<unsigned long>(r.frf), static_cast<unsigned long>(r.frequency_khz));
}

const CliCommand COMMANDS[] = {
    {"help", "help", "list commands", cmd_help},
    {"status", "status", "clock, PWM, mode and button summary", cmd_status},
    {"frequency", "frequency", "PWM frequency read from the timer", cmd_frequency},
    {"duty", "duty", "PWM duty read from the timer", cmd_duty},
    {"set", "set frequency <1-100000>", "change PWM frequency (Hz)", cmd_set},
    {nullptr, "set duty <0-100>", "fixed duty (%), manual mode", nullptr},
    {nullptr, "set brightness <0|20|..|100>", "brightness level through the curve", nullptr},
    {nullptr, "set curve <linear|gamma>", "brightness to duty curve", nullptr},
    {"fade", "fade <linear|gamma|off>", "sweep brightness 0-100-0 %", cmd_fade},
    {"pwm", "pwm <on|off>", "enable or force the output low", cmd_pwm},
    {"clock", "clock", "RCC configuration decoded from registers", cmd_clock},
    {"mco", "mco <on|off>", "SYSCLK/16 on PA8 for the logic analyzer", cmd_mco},
    {"timer", "timer", "timer mode, polarity and registers", cmd_timer},
    {"button", "button", "button state machine counters", cmd_button},
    {"adc", "adc", "VDDA and temperature (internal ADC channels)", cmd_adc},
    {"radio", "radio", "read SX1276 registers over SPI1", cmd_radio},
};

void cmd_help(int, char**) {
    for (const CliCommand& c : COMMANDS) {
        serial_printf("  %-30s %s\r\n", c.usage, c.help);
    }
}

}  // namespace

// -----------------------------------------------------------------------------
void cli_init() {
    serial_printf("\r\nRedsiliente gateway - CLI/PWM (B-L072Z-LRWAN1)\r\n");
    serial_printf("Type 'help' for the command list.\r\n");
}

void cli_print_prompt() { serial_printf("> "); }

void cli_process_line(const char* line) {
    char buffer[SERIAL_LINE_MAX + 1];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* argv[CLI_MAX_ARGS];
    int argc = 0;
    for (char* token = strtok(buffer, " \t"); token != nullptr; token = strtok(nullptr, " \t")) {
        if (argc == static_cast<int>(CLI_MAX_ARGS)) {
            error("too many arguments");
            return;
        }
        argv[argc++] = token;
    }
    if (argc == 0) return;  // linea vacia: solo se repite el prompt

    for (const CliCommand& c : COMMANDS) {
        if (c.name != nullptr && strcmp(c.name, argv[0]) == 0) {
            c.handler(argc, argv);
            return;
        }
    }
    serial_printf("ERROR: unknown command '%s'. Type 'help'.\r\n", argv[0]);
}

void cli_report_overflow() {
    serial_printf("ERROR: line longer than %u characters, discarded\r\n",
                  static_cast<unsigned>(SERIAL_LINE_MAX));
}
