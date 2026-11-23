// =============================================================================
//  brightness.h - Conversion de brillo deseado (%) a duty (por mil)
// -----------------------------------------------------------------------------
//  El ojo responde aprox. de forma logaritmica: un duty lineal concentra el
//  cambio visible en los niveles bajos. La curva gamma D = (B/100)^2.2 reparte
//  mejor los pasos. Es una aproximacion: la respuesta real depende tambien del
//  LED y de su circuito.
// =============================================================================
#pragma once
#include <stdint.h>

enum BrightnessCurve : uint8_t {
    CURVE_LINEAR,
    CURVE_GAMMA,
};

constexpr float BRIGHTNESS_GAMMA = 2.2f;

// Niveles que recorre el boton.
constexpr uint8_t BRIGHTNESS_LEVELS[] = {0, 20, 40, 60, 80, 100};
constexpr uint8_t BRIGHTNESS_LEVEL_COUNT = sizeof(BRIGHTNESS_LEVELS);

// brightness_percent fuera de 0..100 se recorta.
uint32_t brightness_to_duty_permille(uint32_t brightness_percent, BrightnessCurve curve);
const char* brightness_curve_name(BrightnessCurve curve);
bool brightness_is_level(uint32_t brightness_percent);
