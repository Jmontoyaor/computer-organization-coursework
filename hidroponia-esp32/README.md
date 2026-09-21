# Monitor Hidropónico con ESP32

Sistema de medición de variables de cultivos hidropónicos (pH, temperatura, TDS/EC y nivel de agua) basado en ESP32, con un dashboard web en tiempo real servido directamente por el propio dispositivo.

## Cómo funciona

El ESP32 lee los sensores periódicamente, crea su propia red WiFi (Access Point) y sirve una página web con gráficas en vivo vía WebSocket. No requiere conexión a internet ni una app aparte: basta con conectarse a la red WiFi del ESP32 y abrir su IP en el navegador.

```
Sensores ---> ESP32 (ADC + OneWire) ---> WebSocket ---> Dashboard web (HTML/CSS/JS)
```

## Hardware

| Variable         | Sensor                                  | Pin ESP32 |
|-------------------|------------------------------------------|-----------|
| pH                | Sonda de pH analógica (módulo tipo SEN0161 o similar) | GPIO34 (ADC) |
| Temperatura       | DS18B20 (OneWire, resistencia pull-up 4.7kΩ entre datos y VCC) | GPIO4 |
| TDS / EC          | Sensor TDS analógico (módulo tipo Gravity TDS) | GPIO35 (ADC) |
| Nivel de agua     | Sensor resistivo de nivel de agua (analógico) | GPIO32 (ADC) |

Todos los sensores analógicos deben alimentarse a 3.3V (no 5V) para no dañar las entradas ADC del ESP32.

## Firmware

Construido con [PlatformIO](https://platformio.org/).

```bash
# Compilar
pio run

# Subir el firmware
pio run --target upload

# Subir los archivos web (data/) al sistema de archivos LittleFS del ESP32
pio run --target uploadfs

# Ver el monitor serie (para ver la IP asignada y depurar)
pio device monitor
```

Al iniciar, el ESP32 crea la red WiFi **`Hidroponia-ESP32`** (contraseña por defecto `hidroponia123`, configurable en `include/config.h`). Conéctate a esa red desde tu celular o laptop y abre `http://192.168.4.1` en el navegador para ver el dashboard.

## Calibración

Antes de confiar en las lecturas, calibra cada sensor editando `include/config.h`:

- **pH**: sumerge la sonda en solución buffer pH 7.00, espera a que se estabilice y anota el voltaje mostrado por el Serial Monitor; repite con buffer pH 4.00. Actualiza `PH_VOLTAGE_7` y `PH_VOLTAGE_4`.
- **Nivel de agua**: anota la lectura ADC con el sensor seco (`WATER_LEVEL_DRY_ADC`) y completamente sumergido (`WATER_LEVEL_FULL_ADC`).
- **TDS**: usa la compensación por temperatura ya incluida (curva estándar de sensores Gravity TDS); no requiere calibración adicional salvo que uses un módulo distinto.

Los rangos "saludables" que colorean las tarjetas del dashboard (verde = OK, rojo = fuera de rango) se ajustan en `data/app.js`, objeto `ranges`. Los valores por defecto son de referencia para lechuga/hortalizas de hoja; ajústalos al cultivo real.

## Previsualizar el dashboard sin el ESP32

`tools/preview_server.py` sirve `data/` en tu máquina y simula lecturas de sensores por WebSocket, para iterar el diseño del dashboard sin flashear el firmware ni tener el hardware a mano.

```bash
pip install -r tools/requirements.txt
python3 tools/preview_server.py
# abrir http://localhost:8080
```

## Estructura del proyecto

```
hidroponia-esp32/
├── platformio.ini      # Configuración del proyecto y dependencias
├── include/config.h    # Pines, credenciales WiFi y constantes de calibración
├── src/main.cpp         # Firmware: lectura de sensores + servidor web/WebSocket
├── data/                # Dashboard web (se sube al ESP32 vía LittleFS)
│   ├── index.html
│   ├── style.css
│   └── app.js
└── tools/
    ├── preview_server.py   # Servidor local de previsualización (sin hardware)
    └── requirements.txt
```

## Notas

- El dashboard no depende de librerías externas (CDN): todo se sirve localmente, porque el ESP32 en modo Access Point no tiene salida a internet.
- Para usar el ESP32 conectado a una red WiFi existente en vez de crear su propia red, cambia `WiFi.softAP(...)` por `WiFi.begin(ssid, password)` en `src/main.cpp` y considera usar mDNS (`http://hidroponia.local`).
