# Ascensor inteligente ACME S.A. — Actividad 2

Sistema embebido sobre **Arduino UNO** (simulado en **WOKWI**) que implementa un ascensor de cinco plantas con control de movimiento, supervisión ambiental (temperatura e iluminación) y HMI local en LCD.

## Contenido del repositorio

| Archivo | Descripción |
|---|---|
| `sketch.ino` | Código fuente del firmware Arduino |
| `diagram.json` | Esquema del circuito para WOKWI |
| `libraries.txt` | Dependencias que WOKWI instala automáticamente |
| `ALGORITMOS.md` | Explicación de los algoritmos de control |
| `README.md` | Este documento |

## Cómo ejecutarlo

1. Abrir [wokwi.com](https://wokwi.com) → **New Project → Arduino Uno**.
2. Reemplazar el contenido de `sketch.ino`, `diagram.json` y `libraries.txt` por los de este repositorio.
3. Pulsar **Start the simulation**.

## Librerías utilizadas

- `Servo` — control del servomotor por PWM.
- `Wire` + `LiquidCrystal_I2C` — comunicación I2C con el LCD 16×2.
- `DHT` (Adafruit) + `Adafruit Unified Sensor` — lectura del DHT22.
- `IRremote` — decodificación NEC del mando IR.

## Hardware

| Pin | Función | Pin | Función |
|---|---|---|---|
| D2 | Receptor IR | D11 | 74HC595 STCP (latch) |
| D3 | Pulsador planta G | D12 | 74HC595 DS (datos) |
| D4 | DHT22 | D13 | 74HC595 SHCP (reloj) |
| D5-D8, D10 | Pulsadores plantas 1-5 | A0 | LDR |
| D9 | Servo (PWM) | A1-A3 | LEDs CAL / ENF / PRS |
| A4-A5 | LCD SDA / SCL |  |  |

## Estructura del código (`sketch.ino`)

El firmware se organiza en funciones independientes invocadas desde un único `loop()` **no bloqueante** (todos los temporizadores usan `millis()`):

```
setup()                  → inicialización
loop()
 ├─ leerSensores()       → DHT22 + LDR cada 500 ms
 ├─ leerBotones()        → 6 pulsadores con antirrebote
 ├─ leerMandoIR()        → decodificación NEC del mando
 ├─ gestionarMovimiento()→ servo por pisos cada 250 ms
 ├─ controlTemperatura() → algoritmo 3 posiciones + zona muerta
 ├─ controlIluminacion() → escalonado 8 etapas vía 74HC595
 └─ refrescarLCD()       → rotación de 3 vistas cada 1,5 s
```

### Setpoints

- Temperatura: **25 °C** con zona muerta de **±2 °C**.
- Iluminación: **80 %** con 8 escalones (un LED por cada 10 % de déficit).

Los detalles de los algoritmos están en [`ALGORITMOS.md`](ALGORITMOS.md).

## Métricas (compilación verificada con `arduino-cli`)

- Flash: **16 304 B** (50 % de 32 256 B)
- SRAM: **853 B** (42 % de 2 048 B)
- Pines E/S ocupados: **18 / 20**
