// =============================================================================
//  system_clock.h - Configuracion y comprobacion del reloj
// -----------------------------------------------------------------------------
//  Configuracion elegida (system_clock.cpp, SystemClock_Config):
//    HSI16 (16 MHz) -> PLL x4 /2 -> SYSCLK = 32 MHz, AHB /1, APB1 /1, APB2 /1
//  La comprobacion NO usa esa constante: decodifica los registros RCC, FLASH y
//  PWR, y ademas puede sacar SYSCLK/16 por MCO (PA8) para medirlo.
// =============================================================================
#pragma once
#include <stdint.h>

struct ClockStatus {
    const char* sysclk_source;   // "MSI", "HSI16", "HSE", "PLL"
    const char* pll_source;      // "HSI16" o "HSE"
    uint32_t pll_mul;
    uint32_t pll_div;
    uint32_t sysclk_hz;          // calculado desde los registros
    uint32_t hal_sysclk_hz;      // lo que reporta HAL_RCC_GetSysClockFreq()
    uint32_t hclk_hz, pclk1_hz, pclk2_hz;
    uint32_t ahb_div, apb1_div, apb2_div;
    uint32_t apb1_timer_hz;      // TIM2/TIM3: PCLK1, o 2*PCLK1 si APB1 divide
    uint32_t apb2_timer_hz;      // TIM21/TIM22
    const char* lpuart_source;   // reloj del LPUART1 (Serial)
    uint32_t lpuart_brr;
    uint32_t lpuart_baud;        // 256 * f_LPUART / BRR
    uint32_t flash_latency;      // estados de espera
    uint32_t voltage_range;      // 1, 2 o 3
    bool     mco_enabled;
};

void system_clock_get_status(ClockStatus* out);

// MCO en PA8 = SYSCLK / 16 (2 MHz a 32 MHz), facil de medir con Logic 2.
constexpr uint32_t MCO_DIVIDER = 16;
void system_clock_set_mco(bool enabled);
