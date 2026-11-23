// =============================================================================
//  adc_monitor.cpp - VREFINT y sensor de temperatura con la calibracion de fabrica
// =============================================================================
#include "adc_monitor.h"
#include <Arduino.h>
#include "stm32yyxx_ll_adc.h"

namespace {
constexpr uint32_t ADC_BITS = 12;
}  // namespace

void adc_monitor_init() { analogReadResolution(ADC_BITS); }

void adc_monitor_read(AdcReading* out) {
    if (out == nullptr) return;
    out->vrefint_raw = analogRead(AVREF);
    out->vrefint_cal = *VREFINT_CAL_ADDR;
    out->vdda_mv = out->vrefint_raw == 0
        ? 0
        : __LL_ADC_CALC_VREFANALOG_VOLTAGE(out->vrefint_raw, LL_ADC_RESOLUTION_12B);

    out->temperature_raw = analogRead(ATEMP);
    // Interpola entre los dos puntos de fabrica (30 y 130 C) corrigiendo por VDDA.
    out->temperature_c = __LL_ADC_CALC_TEMPERATURE(out->vdda_mv, out->temperature_raw,
                                                   LL_ADC_RESOLUTION_12B);
}
