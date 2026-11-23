// =============================================================================
//  button.h - Boton con antirrebote por marcas de tiempo (sin bloqueos)
// -----------------------------------------------------------------------------
//  RELEASED --nivel activo--> DEBOUNCE_PRESS --estable 20 ms--> PRESSED (evento)
//  PRESSED --nivel inactivo--> DEBOUNCE_RELEASE --estable 20 ms--> RELEASED
//  Si el nivel vuelve atras durante el antirrebote, se regresa al estado previo
//  sin generar evento: eso es un rebote.
// =============================================================================
#pragma once
#include <stdint.h>

enum ButtonState : uint8_t {
    BUTTON_RELEASED,
    BUTTON_DEBOUNCE_PRESS,
    BUTTON_PRESSED,
    BUTTON_DEBOUNCE_RELEASE,
};

constexpr uint32_t BUTTON_DEBOUNCE_MS = 20;

// Configura el pin y toma como nivel de reposo el que tiene al arrancar
// (se supone que nadie pulsa el boton durante el reset).
void button_init(uint32_t pin, bool internal_pullup);

// Llamar en cada vuelta de loop(). Devuelve true UNA vez por pulsacion valida.
bool button_task(uint32_t now_ms);

struct ButtonStatus {
    ButtonState state;
    bool     active_high;      // nivel que significa "pulsado"
    bool     raw_level;        // lectura actual del pin
    uint32_t raw_edges;        // cambios de nivel vistos (incluye rebotes)
    uint32_t press_events;     // pulsaciones validas
    uint32_t rejected_bounces; // cambios descartados por el antirrebote
};
void button_get_status(ButtonStatus* out);
const char* button_state_name(ButtonState state);
