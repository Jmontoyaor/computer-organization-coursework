// =============================================================================
//  radio_probe.h - Lectura de registros del SX1276 por SPI1 (sin RadioLib)
// -----------------------------------------------------------------------------
//  Practica de SPI con el chip de radio del propio proyecto: se reinicia el
//  SX1276 y se leen RegVersion (0x42, debe valer 0x12), RegOpMode (0x01) y
//  RegFrf (0x06-0x08). Primer byte de cada transaccion = direccion con el bit 7
//  en 0 (lectura); el segundo byte saca el dato por MISO.
// =============================================================================
#pragma once
#include <stdint.h>

constexpr uint8_t SX1276_REG_OP_MODE = 0x01;
constexpr uint8_t SX1276_REG_FRF_MSB = 0x06;
constexpr uint8_t SX1276_REG_VERSION = 0x42;
constexpr uint8_t SX1276_EXPECTED_VERSION = 0x12;

struct RadioProbeResult {
    uint8_t  version;
    uint8_t  op_mode;
    uint32_t frf;            // RegFrf de 24 bits
    uint32_t frequency_khz;  // frf * 32 MHz / 2^19
    bool     version_ok;
};

void radio_probe_init();
void radio_probe_read(RadioProbeResult* out);  // ~7 ms por el reset del chip
