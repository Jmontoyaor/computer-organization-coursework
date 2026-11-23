// =============================================================================
//  application.h - Estado de la aplicacion: que duty debe tener el PWM
// -----------------------------------------------------------------------------
//  Tres modos:
//   - LEVEL : el boton recorre 0-20-40-60-80-100 % de brillo con la curva elegida.
//   - MANUAL: el duty lo fija la CLI con "set duty".
//   - FADE  : barrido continuo 0 -> 100 -> 0 % para comparar curvas a simple vista.
//  Este modulo no lee el boton ni la UART: recibe eventos ya interpretados.
// =============================================================================
#pragma once
#include <stdint.h>
#include "brightness.h"

enum AppMode : uint8_t {
    APP_MODE_LEVEL,
    APP_MODE_MANUAL,
    APP_MODE_FADE,
};

void application_init();
void application_task(uint32_t now_ms);  // avanza el fade sin bloquear

void application_on_button_press();      // siguiente nivel de brillo
bool application_set_duty_percent(uint32_t duty_percent);
bool application_set_brightness(uint32_t brightness_percent);
void application_set_curve(BrightnessCurve curve);
void application_start_fade(BrightnessCurve curve);

struct AppStatus {
    AppMode mode;
    BrightnessCurve curve;
    uint32_t brightness_percent;  // ultimo brillo aplicado (LEVEL o FADE)
};
void application_get_status(AppStatus* out);
const char* application_mode_name(AppMode mode);
