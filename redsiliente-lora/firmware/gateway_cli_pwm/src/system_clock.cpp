// =============================================================================
//  system_clock.cpp - Reloj de 32 MHz desde HSI16 + PLL, y su verificacion
// -----------------------------------------------------------------------------
//  Por que 32 MHz: es el maximo del STM32L072 (rango de voltaje 1) y da la mejor
//  resolucion al timer del PWM. Por que HSI16 y no el TCXO de 32 MHz: el TCXO
//  alimenta al SX1276 y solo se enciende con PA12; el HSI16 (+-1 %) basta para
//  115200 baudios. Coste: mas consumo que con MSI; para un nodo a bateria se
//  bajaria el reloj, pero el gateway va conectado al PC por USB.
//  Diferencia con la variante del core: no se enciende el HSI48 (solo sirve
//  para USB, que esta placa no usa).
// =============================================================================
#include "system_clock.h"
#include <Arduino.h>
#include "board_pins.h"

namespace {

constexpr uint32_t HSI16_HZ = 16000000;
constexpr uint32_t MSI_BASE_HZ = 65536;  // rango 0; cada rango duplica
constexpr uint8_t PLL_MUL_TABLE[] = {3, 4, 6, 8, 12, 16, 24, 32, 48};
constexpr uint16_t AHB_DIV_TABLE[] = {2, 4, 8, 16, 64, 128, 256, 512};  // HPRE = 1000..1111
constexpr uint8_t APB_DIV_TABLE[] = {2, 4, 8, 16};                      // PPRE = 100..111
constexpr uint32_t LPUART_BRR_SCALE = 256;

uint32_t field(uint32_t reg, uint32_t mask, uint32_t pos) { return (reg & mask) >> pos; }

uint32_t hsi_hz() {
    // HSIDIVF: el HSI16 puede estar dividido por 4.
    return (RCC->CR & RCC_CR_HSIDIVF) ? HSI16_HZ / 4 : HSI16_HZ;
}

uint32_t ahb_divider(uint32_t hpre) { return hpre < 8 ? 1 : AHB_DIV_TABLE[hpre - 8]; }
uint32_t apb_divider(uint32_t ppre) { return ppre < 4 ? 1 : APB_DIV_TABLE[ppre - 4]; }
uint32_t timer_clock(uint32_t pclk, uint32_t apb_div) { return apb_div == 1 ? pclk : 2 * pclk; }

}  // namespace

// Reemplaza la funcion WEAK de la variante; el core la llama antes de setup().
extern "C" void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {};
    RCC_ClkInitTypeDef clk = {};

    // 32 MHz exige el rango 1 del regulador (1,8 V en el nucleo).
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLMUL = RCC_PLLMUL_4;  // 16 MHz x 4 = 64 MHz (VCO, max 96 MHz)
    osc.PLL.PLLDIV = RCC_PLLDIV_2;  // 64 MHz / 2 = 32 MHz
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;  // con /1 el timer recibe PCLK sin duplicar
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    // Flash: 1 estado de espera por encima de 16 MHz en el rango 1.
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

// -----------------------------------------------------------------------------
void system_clock_get_status(ClockStatus* out) {
    if (out == nullptr) return;
    const uint32_t cfgr = RCC->CFGR;

    // Fuente de PLL y su salida
    const bool pll_from_hse = (cfgr & RCC_CFGR_PLLSRC) != 0;
    const uint32_t pll_in = pll_from_hse ? HSE_VALUE : hsi_hz();
    out->pll_source = pll_from_hse ? "HSE" : "HSI16";
    out->pll_mul = PLL_MUL_TABLE[field(cfgr, RCC_CFGR_PLLMUL, RCC_CFGR_PLLMUL_Pos)];
    out->pll_div = field(cfgr, RCC_CFGR_PLLDIV, RCC_CFGR_PLLDIV_Pos) + 1;  // 01=/2 .. 11=/4

    switch (cfgr & RCC_CFGR_SWS) {
        case RCC_CFGR_SWS_MSI:
            out->sysclk_source = "MSI";
            out->sysclk_hz = MSI_BASE_HZ << field(RCC->ICSCR, RCC_ICSCR_MSIRANGE,
                                                  RCC_ICSCR_MSIRANGE_Pos);
            break;
        case RCC_CFGR_SWS_HSI:
            out->sysclk_source = "HSI16";
            out->sysclk_hz = hsi_hz();
            break;
        case RCC_CFGR_SWS_HSE:
            out->sysclk_source = "HSE";
            out->sysclk_hz = HSE_VALUE;
            break;
        default:
            out->sysclk_source = "PLL";
            out->sysclk_hz = pll_in * out->pll_mul / out->pll_div;
            break;
    }
    out->hal_sysclk_hz = HAL_RCC_GetSysClockFreq();

    out->ahb_div = ahb_divider(field(cfgr, RCC_CFGR_HPRE, RCC_CFGR_HPRE_Pos));
    out->apb1_div = apb_divider(field(cfgr, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_Pos));
    out->apb2_div = apb_divider(field(cfgr, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_Pos));
    out->hclk_hz = out->sysclk_hz / out->ahb_div;
    out->pclk1_hz = out->hclk_hz / out->apb1_div;
    out->pclk2_hz = out->hclk_hz / out->apb2_div;
    out->apb1_timer_hz = timer_clock(out->pclk1_hz, out->apb1_div);
    out->apb2_timer_hz = timer_clock(out->pclk2_hz, out->apb2_div);

    // LPUART1 (Serial): fuente seleccionada en CCIPR y divisor BRR.
    uint32_t lpuart_clock_hz = out->pclk1_hz;
    switch (field(RCC->CCIPR, RCC_CCIPR_LPUART1SEL, RCC_CCIPR_LPUART1SEL_Pos)) {
        case 0: out->lpuart_source = "PCLK1"; break;
        case 1: out->lpuart_source = "SYSCLK"; lpuart_clock_hz = out->sysclk_hz; break;
        case 2: out->lpuart_source = "HSI16"; lpuart_clock_hz = hsi_hz(); break;
        default: out->lpuart_source = "LSE"; lpuart_clock_hz = LSE_VALUE; break;
    }
    out->lpuart_brr = LPUART1->BRR;
    out->lpuart_baud = out->lpuart_brr == 0
        ? 0
        : static_cast<uint32_t>((static_cast<uint64_t>(lpuart_clock_hz) * LPUART_BRR_SCALE +
                                 out->lpuart_brr / 2) / out->lpuart_brr);

    out->flash_latency = (FLASH->ACR & FLASH_ACR_LATENCY) ? 1 : 0;
    out->voltage_range = field(PWR->CR, PWR_CR_VOS, PWR_CR_VOS_Pos);  // 01=1, 10=2, 11=3
    out->mco_enabled = (cfgr & RCC_CFGR_MCOSEL) != 0;
}

void system_clock_set_mco(bool enabled) {
    if (enabled) {
        // Configura PA8 en AF0 (MCO) y selecciona SYSCLK / 16.
        HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_16);
    } else {
        __HAL_RCC_MCO1_CONFIG(RCC_MCO1SOURCE_NOCLOCK, RCC_MCODIV_1);
        pinMode(BoardPins::CLOCK_OUT, INPUT);
    }
}
