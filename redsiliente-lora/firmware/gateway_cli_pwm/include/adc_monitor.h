// =============================================================================
//  adc_monitor.h - ADC con canales internos (sin cableado externo)
// -----------------------------------------------------------------------------
//  VREFINT: referencia interna de ~1,2 V calibrada en fabrica a VDDA = 3,0 V.
//  De su lectura se despeja la tension real de alimentacion:
//      VDDA = 3000 mV * VREFINT_CAL / VREFINT_raw
//  Con VDDA conocida se convierte el sensor de temperatura interno.
// =============================================================================
#pragma once
#include <stdint.h>

struct AdcReading {
    uint32_t vrefint_raw;
    uint32_t vrefint_cal;  // valor de fabrica (memoria de sistema)
    uint32_t vdda_mv;
    uint32_t temperature_raw;
    int32_t  temperature_c;
};

void adc_monitor_init();
void adc_monitor_read(AdcReading* out);
