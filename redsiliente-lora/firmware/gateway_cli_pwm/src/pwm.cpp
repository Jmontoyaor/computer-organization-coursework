// =============================================================================
//  pwm.cpp - PWM por hardware con HardwareTimer (core STM32duino)
// =============================================================================
#include "pwm.h"
#include <Arduino.h>

namespace {

constexpr uint32_t TIMER_COUNTER_MAX = 65536;  // timers de 16 bits del STM32L0
constexpr uint32_t OC_MODE_PWM1 = 6;           // CCMRx.OCxM = 110

HardwareTimer g_timer;
TIM_TypeDef*  g_instance = nullptr;
PinName       g_pin_name = NC;
uint32_t      g_channel = 0;
uint32_t      g_requested_frequency_hz = 0;
uint32_t      g_requested_duty_permille = 0;
bool          g_output_enabled = true;
PwmTiming     g_timing = {0, 0, 0};

const char* timer_name(const TIM_TypeDef* tim) {
#ifdef TIM2
    if (tim == TIM2) return "TIM2";
#endif
#ifdef TIM3
    if (tim == TIM3) return "TIM3";
#endif
#ifdef TIM21
    if (tim == TIM21) return "TIM21";
#endif
#ifdef TIM22
    if (tim == TIM22) return "TIM22";
#endif
    return "TIM?";
}

uint32_t compare_for(uint32_t arr, uint32_t duty_permille) {
    // CCR = D * (ARR + 1). Con CCR = ARR + 1 la salida queda siempre en alto (100 %).
    const uint64_t period = static_cast<uint64_t>(arr) + 1;
    return static_cast<uint32_t>((period * duty_permille + PWM_DUTY_FULL_SCALE / 2) /
                                 PWM_DUTY_FULL_SCALE);
}

void apply_timing(const PwmTiming& t) {
    g_timer.setPrescaleFactor(t.psc + 1);         // escribe PSC = factor - 1
    g_timer.setOverflow(t.arr + 1, TICK_FORMAT);  // escribe ARR = valor - 1
    if (g_output_enabled) {
        g_timer.setCaptureCompare(g_channel, t.ccr, TICK_COMPARE_FORMAT);
    }
    // PSC tiene buffer: solo se carga en el siguiente evento de actualizacion.
    // refresh() genera ese evento ya y pone CNT a 0. Asi se evita que, al bajar
    // ARR, el contador quede por encima y tenga que llegar hasta 65535.
    g_timer.refresh();
    g_timing = t;
}

volatile uint32_t* ccr_register(uint32_t channel) {
    return &g_instance->CCR1 + (channel - 1);  // CCR1..CCR4 son contiguos
}

}  // namespace

// -----------------------------------------------------------------------------
bool pwm_compute_timing(uint32_t timer_clock_hz, uint32_t frequency_hz,
                        uint32_t duty_permille, PwmTiming* out) {
    if (frequency_hz == 0 || duty_permille > PWM_DUTY_FULL_SCALE || out == nullptr) return false;

    // Cuentas por periodo si PSC = 0, redondeado al entero mas cercano.
    const uint64_t total_counts = (static_cast<uint64_t>(timer_clock_hz) + frequency_hz / 2) /
                                  frequency_hz;
    const uint64_t divider = (total_counts + TIMER_COUNTER_MAX - 1) / TIMER_COUNTER_MAX;  // PSC+1
    if (divider == 0 || divider > TIMER_COUNTER_MAX) return false;

    const uint64_t period = (total_counts + divider / 2) / divider;  // ARR+1
    if (period < 2 || period > TIMER_COUNTER_MAX) return false;      // sin resolucion util

    out->psc = static_cast<uint32_t>(divider - 1);
    out->arr = static_cast<uint32_t>(period - 1);
    out->ccr = compare_for(out->arr, duty_permille);
    return true;
}

// -----------------------------------------------------------------------------
bool pwm_init(uint32_t pin, uint32_t frequency_hz, uint32_t duty_permille) {
    g_pin_name = digitalPinToPinName(pin);
    g_instance = static_cast<TIM_TypeDef*>(pinmap_peripheral(g_pin_name, PinMap_TIM));
    if (g_instance == nullptr) return false;  // el pin no tiene funcion de timer
    g_channel = STM_PIN_CHANNEL(pinmap_function(g_pin_name, PinMap_TIM));

    g_timer.setup(g_instance);
    // PWM modo 1: salida activa mientras CNT < CCR. Conteo ascendente y alineado
    // al flanco: es el valor por defecto de HardwareTimer (CR1.DIR = 0, CMS = 00).
    g_timer.setMode(g_channel, TIMER_OUTPUT_COMPARE_PWM1, g_pin_name);
    g_timer.setPreloadEnable(true);  // ARR con buffer: los cambios entran al final del periodo

    PwmTiming timing;
    if (!pwm_compute_timing(g_timer.getTimerClkFreq(), frequency_hz, duty_permille, &timing)) {
        return false;
    }
    g_requested_frequency_hz = frequency_hz;
    g_requested_duty_permille = duty_permille;
    apply_timing(timing);
    g_timer.resume();
    return true;
}

bool pwm_set_frequency_hz(uint32_t frequency_hz) {
    if (frequency_hz < PWM_MIN_FREQUENCY_HZ || frequency_hz > PWM_MAX_FREQUENCY_HZ) return false;
    PwmTiming timing;
    // El duty se conserva: se recalcula CCR para el nuevo ARR.
    if (!pwm_compute_timing(g_timer.getTimerClkFreq(), frequency_hz, g_requested_duty_permille,
                            &timing)) {
        return false;
    }
    g_requested_frequency_hz = frequency_hz;
    apply_timing(timing);
    return true;
}

bool pwm_set_duty_permille(uint32_t duty_permille) {
    if (duty_permille > PWM_DUTY_FULL_SCALE) return false;
    g_requested_duty_permille = duty_permille;
    g_timing.ccr = compare_for(g_timing.arr, duty_permille);
    // CCR tiene buffer (OCxPE): el nuevo duty empieza con el siguiente periodo,
    // sin pulsos cortados. No hace falta reiniciar el contador.
    if (g_output_enabled) g_timer.setCaptureCompare(g_channel, g_timing.ccr, TICK_COMPARE_FORMAT);
    return true;
}

void pwm_set_output_enabled(bool enabled) {
    if (enabled == g_output_enabled) return;
    g_output_enabled = enabled;
    if (enabled) {
        g_timer.setMode(g_channel, TIMER_OUTPUT_COMPARE_PWM1, g_pin_name);
        g_timer.setCaptureCompare(g_channel, g_timing.ccr, TICK_COMPARE_FORMAT);
    } else {
        // Nivel inactivo forzado: el pin queda en bajo y el timer sigue contando,
        // asi al reactivar no hay que reconfigurar PSC/ARR.
        g_timer.setMode(g_channel, TIMER_OUTPUT_COMPARE_FORCED_INACTIVE, g_pin_name);
    }
}

// -----------------------------------------------------------------------------
void pwm_get_status(PwmStatus* out) {
    if (out == nullptr || g_instance == nullptr) return;
    const uint32_t ch_index = g_channel - 1;
    const uint32_t ccmr = (ch_index < 2) ? g_instance->CCMR1 : g_instance->CCMR2;
    const uint32_t ccmr_shift = (ch_index % 2) ? 8 : 0;  // canal par: byte alto

    out->timer_name = timer_name(g_instance);
    out->channel = g_channel;
    out->timer_clock_hz = g_timer.getTimerClkFreq();
    out->psc = g_instance->PSC;
    out->arr = g_instance->ARR;
    out->ccr = *ccr_register(g_channel);
    out->requested_frequency_hz = g_requested_frequency_hz;
    out->requested_duty_permille = g_requested_duty_permille;

    const uint64_t counts = static_cast<uint64_t>(out->psc + 1) * (out->arr + 1);
    out->actual_frequency_mhz = static_cast<uint32_t>(
        (static_cast<uint64_t>(out->timer_clock_hz) * 1000 + counts / 2) / counts);
    const uint32_t effective_ccr = (out->ccr > out->arr + 1) ? out->arr + 1 : out->ccr;
    out->actual_duty_permille = static_cast<uint32_t>(
        (static_cast<uint64_t>(effective_ccr) * PWM_DUTY_FULL_SCALE + (out->arr + 1) / 2) /
        (out->arr + 1));

    out->oc_mode = (ccmr >> (ccmr_shift + 4)) & 0x7;  // OCxM
    out->preload_enabled = (ccmr >> (ccmr_shift + 3)) & 0x1;  // OCxPE
    out->output_enabled = g_output_enabled && out->oc_mode == OC_MODE_PWM1;
    out->up_counting = (g_instance->CR1 & TIM_CR1_DIR) == 0;
    out->edge_aligned = (g_instance->CR1 & TIM_CR1_CMS) == 0;
    out->active_high = (g_instance->CCER & (TIM_CCER_CC1P << (4 * ch_index))) == 0;
}
