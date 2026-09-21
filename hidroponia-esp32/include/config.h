#pragma once

// --- Red WiFi que crea el ESP32 (modo Access Point) ---
#define AP_SSID "Hidroponia-ESP32"
#define AP_PASSWORD "hidroponia123"

// --- Pines de sensores ---
#define PH_PIN 34           // Sensor de pH (analogico)
#define TDS_PIN 35          // Sensor TDS/EC (analogico)
#define WATER_LEVEL_PIN 32  // Sensor de nivel de agua (analogico)
#define ONE_WIRE_PIN 4      // Bus OneWire para el DS18B20 (temperatura)

// --- Calibracion del sensor de pH (calibracion de 2 puntos) ---
// Sumerge la sonda en solucion buffer pH 7.00, espera a que se estabilice
// y anota el voltaje promedio (ver Serial Monitor). Repite con buffer pH 4.00.
#define PH_VOLTAGE_7 2.50f
#define PH_VOLTAGE_4 3.10f

// --- Calibracion del sensor de nivel de agua ---
// Lectura ADC con el sensor seco y con el sensor completamente sumergido.
#define WATER_LEVEL_DRY_ADC 500
#define WATER_LEVEL_FULL_ADC 3000

// --- ADC del ESP32 ---
#define ADC_VREF 3.3f
#define ADC_RESOLUTION 4095.0f

// --- Intervalo de muestreo y difusion por WebSocket (ms) ---
#define SAMPLE_INTERVAL_MS 2000
