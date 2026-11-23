// =============================================================================
//  application.cpp - Logica de brillo; el PWM solo recibe duties
// =============================================================================
#include "application.h"
#include "pwm.h"

namespace {

constexpr uint32_t FADE_STEP_MS = 20;  // 100 pasos de subida + 100 de bajada = 4 s por ciclo

AppMode         g_mode = APP_MODE_LEVEL;
BrightnessCurve g_curve = CURVE_GAMMA;
uint8_t         g_level_index = 0;
uint32_t        g_brightness_percent = 0;
uint32_t        g_last_fade_step_ms = 0;
int32_t         g_fade_direction = 1;

void apply_brightness(uint32_t brightness_percent) {
    g_brightness_percent = brightness_percent;
    pwm_set_duty_permille(brightness_to_duty_permille(brightness_percent, g_curve));
}

}  // namespace

void application_init() {
    g_mode = APP_MODE_LEVEL;
    g_level_index = 0;
    apply_brightness(BRIGHTNESS_LEVELS[g_level_index]);
}

void application_task(uint32_t now_ms) {
    if (g_mode != APP_MODE_FADE || now_ms - g_last_fade_step_ms < FADE_STEP_MS) return;
    g_last_fade_step_ms = now_ms;

    int32_t next = static_cast<int32_t>(g_brightness_percent) + g_fade_direction;
    if (next >= 100) {
        next = 100;
        g_fade_direction = -1;
    } else if (next <= 0) {
        next = 0;
        g_fade_direction = 1;
    }
    apply_brightness(static_cast<uint32_t>(next));
}

void application_on_button_press() {
    // Desde MANUAL o FADE, la primera pulsacion vuelve a los niveles en 0 %.
    if (g_mode == APP_MODE_LEVEL) {
        g_level_index = (g_level_index + 1) % BRIGHTNESS_LEVEL_COUNT;
    } else {
        g_mode = APP_MODE_LEVEL;
        g_level_index = 0;
    }
    apply_brightness(BRIGHTNESS_LEVELS[g_level_index]);
}

bool application_set_duty_percent(uint32_t duty_percent) {
    if (duty_percent > 100) return false;
    g_mode = APP_MODE_MANUAL;
    return pwm_set_duty_permille(duty_percent * 10);
}

bool application_set_brightness(uint32_t brightness_percent) {
    for (uint8_t i = 0; i < BRIGHTNESS_LEVEL_COUNT; ++i) {
        if (BRIGHTNESS_LEVELS[i] == brightness_percent) {
            g_mode = APP_MODE_LEVEL;
            g_level_index = i;
            apply_brightness(brightness_percent);
            return true;
        }
    }
    return false;
}

void application_set_curve(BrightnessCurve curve) {
    g_curve = curve;
    if (g_mode != APP_MODE_MANUAL) apply_brightness(g_brightness_percent);
}

void application_start_fade(BrightnessCurve curve) {
    g_curve = curve;
    g_mode = APP_MODE_FADE;
    g_fade_direction = 1;
    apply_brightness(0);
}

void application_get_status(AppStatus* out) {
    if (out == nullptr) return;
    out->mode = g_mode;
    out->curve = g_curve;
    out->brightness_percent = g_brightness_percent;
}

const char* application_mode_name(AppMode mode) {
    switch (mode) {
        case APP_MODE_LEVEL:  return "level (button)";
        case APP_MODE_MANUAL: return "manual duty";
        case APP_MODE_FADE:   return "fade";
    }
    return "?";
}
