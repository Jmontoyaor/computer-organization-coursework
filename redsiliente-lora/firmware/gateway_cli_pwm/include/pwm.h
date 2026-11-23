// =============================================================================
//  pwm.h - Salida PWM por hardware (timer en modo PWM1, conteo ascendente)
// -----------------------------------------------------------------------------
//  f_PWM = f_TIM / ((PSC + 1) * (ARR + 1))      D = CCR / (ARR + 1)
//  El duty se maneja en por mil (0..1000) para poder representar los niveles
//  pequenos de la curva gamma (2,9 % = 29 por mil).
// =============================================================================
#pragma once
#include <stdint.h>
#include <stddef.h>

constexpr uint32_t PWM_MIN_FREQUENCY_HZ = 1;
constexpr uint32_t PWM_MAX_FREQUENCY_HZ = 100000;
constexpr uint32_t PWM_DUTY_FULL_SCALE  = 1000;  // 100,0 %

// Registros calculados para una frecuencia y un duty.
struct PwmTiming {
    uint32_t psc;  // valor del registro PSC (el divisor real es PSC + 1)
    uint32_t arr;  // valor del registro ARR (el periodo en cuentas es ARR + 1)
    uint32_t ccr;  // valor del registro CCR del canal
};

// Calculo puro (sin hardware): elige el PSC mas pequeno que deja ARR en 16 bits,
// para tener la mayor resolucion de duty posible. false si la frecuencia no se
// puede generar con este reloj.
bool pwm_compute_timing(uint32_t timer_clock_hz, uint32_t frequency_hz,
                        uint32_t duty_permille, PwmTiming* out);

// Configura el timer del pin (se busca en el mapa de pines del core).
bool pwm_init(uint32_t pin, uint32_t frequency_hz, uint32_t duty_permille);

// Cambian la salida en caliente. Devuelven false si el valor esta fuera de rango;
// en ese caso no se toca el timer.
bool pwm_set_frequency_hz(uint32_t frequency_hz);
bool pwm_set_duty_permille(uint32_t duty_permille);
void pwm_set_output_enabled(bool enabled);

// Lectura del estado real: todo sale de los registros del timer.
struct PwmStatus {
    const char* timer_name;    // "TIM2", "TIM3", "TIM21", "TIM22"
    uint32_t channel;          // 1..4
    uint32_t timer_clock_hz;   // reloj de entrada del timer
    uint32_t psc, arr, ccr;    // registros
    uint32_t requested_frequency_hz;
    uint32_t requested_duty_permille;
    uint32_t actual_frequency_mhz;   // en milihercios, para no usar float
    uint32_t actual_duty_permille;
    bool     output_enabled;
    bool     up_counting;      // CR1.DIR = 0
    bool     edge_aligned;     // CR1.CMS = 00
    bool     active_high;      // CCER.CCxP = 0
    uint32_t oc_mode;          // CCMRx.OCxM (6 = PWM modo 1)
    bool     preload_enabled;  // CCMRx.OCxPE
};
void pwm_get_status(PwmStatus* out);
