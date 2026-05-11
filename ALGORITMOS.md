# Algoritmos implementados

Tres lazos de control conviven en el `loop()` principal, multiplexados con `millis()` para que ninguno bloquee al resto.

---

## 1. Movimiento del ascensor

Sistema discreto de 6 estados (G, 1, 2, 3, 4, 5) mapeados linealmente al rango angular del servomotor SG90.

```
ángulo = map(planta, 0, 5, 0, 180)
```

Cada **250 ms** se avanza un piso en dirección al destino. El ángulo se actualiza al final de cada paso, produciendo un desplazamiento perceptible (~1,25 s entre G y planta 5).

El destino se puede fijar de dos formas:
- Pulsadores físicos en cada planta (`INPUT_PULLUP`, antirrebote 200 ms).
- Mando IR (protocolo NEC, teclas 0-5).

---

## 2. Control de temperatura — 3 posiciones con zona muerta

Algoritmo discontinuo de la figura 3 del enunciado, con setpoint 25 °C y zona muerta ±2 °C:

```
if  T ≥ SP + ZM    →   enfriar (LED azul ON)
if  T ≤ SP − ZM    →   calentar (LED rojo ON)
en otro caso       →   ambos OFF (zona muerta)
```

| T (°C) | Acción |
|---|---|
| ≤ 23 | CALENTAR |
| 23-27 | OFF (zona muerta) |
| ≥ 27 | ENFRIAR |

El lazo se ejecuta cada **500 ms**, coincidiendo con el retardo del proceso térmico simulado. Los LEDs CAL / ENF representan las dos electroválvulas (caliente y fría).

**¿Por qué este algoritmo?** Permite acciones bidireccionales (calentar y enfriar) y la zona muerta evita oscilaciones cerca del setpoint. Es el más simple compatible con un control térmico bidireccional.

---

## 3. Control de iluminación — escalonado on-off de 8 etapas

La respuesta se discretiza en 8 niveles. El número de LEDs encendidos es proporcional al **déficit** respecto al setpoint 80 %:

```
si  luz < 80 %  →   LEDs ON = (80 − luz) / 10
si  luz ≥ 80 %  →   LEDs ON = 0
```

| Luz medida | LEDs ON |
|---|---|
| 0 % | 8 |
| 30 % | 5 |
| 50 % | 3 |
| 70 % | 1 |
| ≥ 80 % | 0 |

Los 8 LEDs se gobiernan con un **registro de desplazamiento 74HC595**, que reduce el control de 8 salidas a 3 pines del Arduino (DS, SHCP, STCP). El patrón se construye con `(1 << ledsLuz) − 1` y se envía con `shiftOut(MSBFIRST)`; el latch del 595 actualiza las 8 salidas de forma atómica.

---

## 4. HMI — rotación automática de vistas

El LCD I2C 16×2 muestra **tres vistas que rotan cada 1,5 s**, sin necesidad de pulsadores adicionales.

Cada vista combina **medida + setpoint + acción de control**, de modo que el usuario percibe en tiempo real el efecto del controlador.

---

## Resumen de periodos

| Tarea | Periodo | Motivo |
|---|---|---|
| Lectura sensores | 500 ms | Tiempo mínimo de muestreo del DHT22 |
| Control temperatura | 500 ms | Retardo del proceso térmico simulado |
| Control iluminación | continuo | Es una asignación combinacional, no requiere periodo |
| Movimiento servo | 250 ms / piso | Velocidad realista del SG90 |
| Refresco LCD | 1500 ms | Tiempo de lectura humano cómodo |
| Antirrebote pulsadores | 200 ms | Mayor que el rebote típico (5-20 ms) |
