#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

#include "config.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature tempSensor(&oneWire);

unsigned long lastSample = 0;

float readVoltage(int pin, uint8_t samples = 20) {
  uint32_t total = 0;
  for (uint8_t i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(5);
  }
  float avgAdc = total / (float)samples;
  return avgAdc * (ADC_VREF / ADC_RESOLUTION);
}

float readPH() {
  float voltage = readVoltage(PH_PIN);
  // Recta que pasa por los dos puntos de calibracion (pH 4 y pH 7).
  float slope = (7.0f - 4.0f) / (PH_VOLTAGE_7 - PH_VOLTAGE_4);
  return 7.0f + slope * (voltage - PH_VOLTAGE_7);
}

float readTDS(float temperatureC) {
  float voltage = readVoltage(TDS_PIN);
  // Compensacion por temperatura y curva de conversion del sensor
  // Gravity TDS (DFRobot), referencia estandar del fabricante.
  float compensationCoefficient = 1.0f + 0.02f * (temperatureC - 25.0f);
  float compensatedVoltage = voltage / compensationCoefficient;
  float tds = (133.42f * pow(compensatedVoltage, 3)
             - 255.86f * pow(compensatedVoltage, 2)
             + 857.39f * compensatedVoltage) * 0.5f;
  return max(tds, 0.0f);
}

float readWaterLevel() {
  uint32_t total = 0;
  const uint8_t samples = 20;
  for (uint8_t i = 0; i < samples; i++) {
    total += analogRead(WATER_LEVEL_PIN);
    delay(5);
  }
  float avgAdc = total / (float)samples;
  float percent = (avgAdc - WATER_LEVEL_DRY_ADC) * 100.0f
                 / (WATER_LEVEL_FULL_ADC - WATER_LEVEL_DRY_ADC);
  return constrain(percent, 0.0f, 100.0f);
}

void broadcastReadings() {
  tempSensor.requestTemperatures();
  float temperature = tempSensor.getTempCByIndex(0);
  bool hasTemperature = temperature != DEVICE_DISCONNECTED_C;

  float ph = readPH();
  float tds = readTDS(hasTemperature ? temperature : 25.0f);
  float level = readWaterLevel();

  JsonDocument doc;
  doc["ph"] = roundf(ph * 100) / 100.0f;
  if (hasTemperature) {
    doc["temperature"] = roundf(temperature * 10) / 10.0f;
  } else {
    doc["temperature"] = nullptr;
  }
  doc["tds"] = roundf(tds);
  doc["waterLevel"] = roundf(level);
  doc["uptime"] = millis() / 1000;

  String payload;
  serializeJson(doc, payload);
  ws.textAll(payload);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Cliente WS #%u conectado\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Cliente WS #%u desconectado\n", client->id());
  }
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  tempSensor.begin();

  if (!LittleFS.begin(true)) {
    Serial.println("Error montando LittleFS");
  }

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("Access Point iniciado. Conectate a \"");
  Serial.print(AP_SSID);
  Serial.print("\" y abre http://");
  Serial.println(WiFi.softAPIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.begin();
}

void loop() {
  ws.cleanupClients();

  if (millis() - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = millis();
    broadcastReadings();
  }
}
