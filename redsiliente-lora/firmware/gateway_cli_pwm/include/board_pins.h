// =============================================================================
//  board_pins.h - Pines de la B-L072Z-LRWAN1 usados por este firmware
// -----------------------------------------------------------------------------
//  Fuente: UM2115 (manual de la placa) y el BSP de ST (b-l072z-lrwan1.h), los
//  mismos que usa firmware/gateway_receiver.
// =============================================================================
#pragma once
#include <Arduino.h>

namespace BoardPins {

// LD1 verde. Se usa como salida PWM porque PB5 tiene funcion alternativa de
// timer; el firmware consulta el mapa de pines del core (PinMap_TIM) para saber
// que timer y canal le corresponden, en lugar de suponerlo.
// LD2 (PA5) NO se usa: comparte pin con DIO4 del SX1276.
constexpr uint32_t PWM_LED = PB5;

// B1 USER. El nivel activo se detecta al arrancar (ver button.cpp).
constexpr uint32_t USER_BUTTON = PB2;

// Salida de reloj MCO (PA8, AF0) para medir SYSCLK con el analizador logico.
constexpr uint32_t CLOCK_OUT = PA8;

// SX1276 dentro del modulo Murata (bus SPI1 interno).
constexpr uint32_t RADIO_MOSI  = PA7;
constexpr uint32_t RADIO_MISO  = PA6;
constexpr uint32_t RADIO_SCK   = PB3;
constexpr uint32_t RADIO_NSS   = PA15;
constexpr uint32_t RADIO_RESET = PC0;
constexpr uint32_t RADIO_TCXO_VCC = PA12;

// Conmutador de antena: se dejan en bajo (sin camino RF activo) mientras solo
// se leen registros.
constexpr uint32_t ANT_SW_RX       = PA1;
constexpr uint32_t ANT_SW_TX_BOOST = PC1;
constexpr uint32_t ANT_SW_TX_RFO   = PC2;

}  // namespace BoardPins
