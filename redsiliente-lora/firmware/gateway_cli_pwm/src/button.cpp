// =============================================================================
//  button.cpp - Maquina de estados del boton
// =============================================================================
#include "button.h"
#include <Arduino.h>

namespace {

uint32_t    g_pin = 0;
bool        g_active_high = true;
bool        g_last_raw = false;
ButtonState g_state = BUTTON_RELEASED;
uint32_t    g_state_since_ms = 0;
uint32_t    g_raw_edges = 0;
uint32_t    g_press_events = 0;
uint32_t    g_rejected = 0;

bool is_active(bool raw_level) { return raw_level == g_active_high; }

void enter(ButtonState next, uint32_t now_ms) {
    g_state = next;
    g_state_since_ms = now_ms;
}

}  // namespace

void button_init(uint32_t pin, bool internal_pullup) {
    g_pin = pin;
    pinMode(pin, internal_pullup ? INPUT_PULLUP : INPUT);
    delayMicroseconds(50);  // que la entrada se asiente tras activar el pull-up
    const bool idle_level = digitalRead(pin) == HIGH;
    g_active_high = !idle_level;
    g_last_raw = idle_level;
    enter(BUTTON_RELEASED, millis());
}

bool button_task(uint32_t now_ms) {
    const bool raw = digitalRead(g_pin) == HIGH;
    if (raw != g_last_raw) {
        g_raw_edges++;
        g_last_raw = raw;
    }
    const bool active = is_active(raw);
    const bool settled = (now_ms - g_state_since_ms) >= BUTTON_DEBOUNCE_MS;

    switch (g_state) {
        case BUTTON_RELEASED:
            if (active) enter(BUTTON_DEBOUNCE_PRESS, now_ms);
            break;

        case BUTTON_DEBOUNCE_PRESS:
            if (!active) {
                g_rejected++;
                enter(BUTTON_RELEASED, now_ms);
            } else if (settled) {
                enter(BUTTON_PRESSED, now_ms);
                g_press_events++;
                return true;  // el evento sale al confirmar la pulsacion
            }
            break;

        case BUTTON_PRESSED:
            if (!active) enter(BUTTON_DEBOUNCE_RELEASE, now_ms);
            break;

        case BUTTON_DEBOUNCE_RELEASE:
            if (active) {
                g_rejected++;
                enter(BUTTON_PRESSED, now_ms);
            } else if (settled) {
                enter(BUTTON_RELEASED, now_ms);
            }
            break;
    }
    return false;
}

void button_get_status(ButtonStatus* out) {
    if (out == nullptr) return;
    out->state = g_state;
    out->active_high = g_active_high;
    out->raw_level = g_last_raw;
    out->raw_edges = g_raw_edges;
    out->press_events = g_press_events;
    out->rejected_bounces = g_rejected;
}

const char* button_state_name(ButtonState state) {
    switch (state) {
        case BUTTON_RELEASED:         return "RELEASED";
        case BUTTON_DEBOUNCE_PRESS:   return "DEBOUNCE_PRESS";
        case BUTTON_PRESSED:          return "PRESSED";
        case BUTTON_DEBOUNCE_RELEASE: return "DEBOUNCE_RELEASE";
    }
    return "?";
}
