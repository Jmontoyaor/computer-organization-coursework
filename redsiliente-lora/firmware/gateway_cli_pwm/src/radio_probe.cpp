// =============================================================================
//  radio_probe.cpp - SPI1 interno del modulo Murata
// =============================================================================
#include "radio_probe.h"
#include "board_pins.h"
#include <SPI.h>

namespace {

constexpr uint32_t SPI_CLOCK_HZ = 2000000;      // igual que el gateway LoRa
constexpr uint32_t RESET_PULSE_US = 200;        // hoja de datos: > 100 us en bajo
constexpr uint32_t RESET_READY_MS = 6;          // hoja de datos: 5 ms hasta estar listo
constexpr uint32_t TCXO_STARTUP_MS = 5;
constexpr uint64_t FXOSC_HZ = 32000000;
constexpr uint32_t FRF_SHIFT = 19;              // F_STEP = FXOSC / 2^19

SPIClass g_spi(BoardPins::RADIO_MOSI, BoardPins::RADIO_MISO, BoardPins::RADIO_SCK);
const SPISettings g_settings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE0);

uint8_t read_register(uint8_t address) {
    g_spi.beginTransaction(g_settings);
    digitalWrite(BoardPins::RADIO_NSS, LOW);
    g_spi.transfer(address & 0x7F);  // bit 7 = 0: lectura
    const uint8_t value = g_spi.transfer(0x00);
    digitalWrite(BoardPins::RADIO_NSS, HIGH);
    g_spi.endTransaction();
    return value;
}

}  // namespace

void radio_probe_init() {
    // Conmutador de antena sin camino activo y chip deseleccionado.
    for (uint32_t pin : {BoardPins::ANT_SW_RX, BoardPins::ANT_SW_TX_BOOST,
                         BoardPins::ANT_SW_TX_RFO}) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    pinMode(BoardPins::RADIO_NSS, OUTPUT);
    digitalWrite(BoardPins::RADIO_NSS, HIGH);
    pinMode(BoardPins::RADIO_RESET, OUTPUT);
    digitalWrite(BoardPins::RADIO_RESET, HIGH);
    pinMode(BoardPins::RADIO_TCXO_VCC, OUTPUT);
    digitalWrite(BoardPins::RADIO_TCXO_VCC, HIGH);  // el SX1276 no tiene reloj sin el TCXO
    g_spi.begin();
}

void radio_probe_read(RadioProbeResult* out) {
    if (out == nullptr) return;
    // Las esperas son cortas y solo ocurren al pedir el comando "radio" por CLI.
    delay(TCXO_STARTUP_MS);
    digitalWrite(BoardPins::RADIO_RESET, LOW);
    delayMicroseconds(RESET_PULSE_US);
    digitalWrite(BoardPins::RADIO_RESET, HIGH);
    delay(RESET_READY_MS);

    out->version = read_register(SX1276_REG_VERSION);
    out->op_mode = read_register(SX1276_REG_OP_MODE);
    out->frf = (static_cast<uint32_t>(read_register(SX1276_REG_FRF_MSB)) << 16) |
               (static_cast<uint32_t>(read_register(SX1276_REG_FRF_MSB + 1)) << 8) |
               read_register(SX1276_REG_FRF_MSB + 2);
    out->frequency_khz = static_cast<uint32_t>(((out->frf * FXOSC_HZ) >> FRF_SHIFT) / 1000);
    out->version_ok = out->version == SX1276_EXPECTED_VERSION;
}
