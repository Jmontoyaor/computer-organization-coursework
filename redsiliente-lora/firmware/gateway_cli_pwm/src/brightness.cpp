// =============================================================================
//  brightness.cpp - Curvas lineal y gamma
// =============================================================================
#include "brightness.h"
#include "pwm.h"
#include <math.h>

uint32_t brightness_to_duty_permille(uint32_t brightness_percent, BrightnessCurve curve) {
    if (brightness_percent >= 100) return PWM_DUTY_FULL_SCALE;  // limite exacto en 100 %
    if (brightness_percent == 0) return 0;                       // y en 0 %
    if (curve == CURVE_LINEAR) return brightness_percent * 10;

    const float normalized = static_cast<float>(brightness_percent) / 100.0f;
    const float duty = powf(normalized, BRIGHTNESS_GAMMA) * PWM_DUTY_FULL_SCALE;
    return static_cast<uint32_t>(duty + 0.5f);
}

const char* brightness_curve_name(BrightnessCurve curve) {
    return curve == CURVE_GAMMA ? "gamma 2.2" : "linear";
}

bool brightness_is_level(uint32_t brightness_percent) {
    for (uint8_t level : BRIGHTNESS_LEVELS) {
        if (level == brightness_percent) return true;
    }
    return false;
}
