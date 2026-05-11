// *****************************************************************************
// *  Equipos e Instrumentacion Electronica
// *  Actividad 2 - Desarrollo de lógica de control y actuación para ascensor 
// *                inteligente en entorno industrial ACME S.A.
// *
// *  Antonio Martinez Corral
// *****************************************************************************

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <IRremote.hpp>

// ---------------- Pines ----------------
#define PIN_IR         2
#define PIN_BOT_G      3
#define PIN_DHT        4
#define PIN_BOT_1      5
#define PIN_BOT_2      6
#define PIN_BOT_3      7
#define PIN_BOT_4      8
#define PIN_SERVO      9
#define PIN_BOT_5      10
#define PIN_595_LATCH  11
#define PIN_595_DATA   12
#define PIN_595_CLK    13
#define PIN_LDR        A0
#define PIN_LED_CAL    A1
#define PIN_LED_ENF    A2
#define PIN_LED_PRES   A3

const byte BOT_PLANTA[6] = {PIN_BOT_G, PIN_BOT_1, PIN_BOT_2,
                            PIN_BOT_3, PIN_BOT_4, PIN_BOT_5};

// ---------------- Parámetros de control ----------------
const float TEMP_SETPOINT = 25.0;  // °C
const float TEMP_ZM       = 2.0;   // zona muerta
const byte  LUZ_SETPOINT  = 80;    // %
const byte  LUZ_ETAPAS    = 8;     // 8 LEDs del 74HC595

const unsigned long T_LECT = 500;
const unsigned long T_CTRL = 500;
const unsigned long T_LCD  = 1500;
const unsigned long T_PASO = 250;

// ---------------- Objetos y estado ----------------
Servo  servoCabina;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT    dht(PIN_DHT, DHT22);

byte  plantaActual = 0, plantaDestino = 0, personas = 0;
float tempMedida = 25.0, humMedida = 50.0;
byte  luzPct = 0;
bool  estadoCal = false, estadoEnf = false;
byte  ledsLuz = 0;

unsigned long tLect = 0, tCtrl = 0, tLcd = 0, tPaso = 0;
byte vistaLcd = 0;

// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED_CAL,   OUTPUT);
  pinMode(PIN_LED_ENF,   OUTPUT);
  pinMode(PIN_LED_PRES,  OUTPUT);
  pinMode(PIN_595_DATA,  OUTPUT);
  pinMode(PIN_595_LATCH, OUTPUT);
  pinMode(PIN_595_CLK,   OUTPUT);
  for (byte i = 0; i < 6; i++) pinMode(BOT_PLANTA[i], INPUT_PULLUP);

  servoCabina.attach(PIN_SERVO);
  servoCabina.write(0);
  lcd.init(); lcd.backlight();
  dht.begin();
  // D13 (LED integrado) coincide con SHCP del 74HC595: el feedback
  // generaría pulsos de reloj falsos sobre el registro.
  IrReceiver.begin(PIN_IR, DISABLE_LED_FEEDBACK);

  lcd.setCursor(0, 0); lcd.print(F("ACME Ascensor"));
  lcd.setCursor(0, 1); lcd.print(F("Sistema listo"));
  escribir74HC(0);
  Serial.println(F("ACME Ascensor listo"));
}

// ============================================================
void loop() {
  leerSensores();
  leerBotones();
  leerMandoIR();
  gestionarMovimiento();
  controlTemperatura();
  controlIluminacion();
  digitalWrite(PIN_LED_PRES, personas > 0);
  refrescarLCD();
}

// ---------------- Sensores ----------------
void leerSensores() {
  if (millis() - tLect < T_LECT) return;
  tLect = millis();
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) tempMedida = t;
  if (!isnan(h)) humMedida  = h;
  luzPct = map(analogRead(PIN_LDR), 0, 1023, 0, 100);
}

// ---------------- Entradas ----------------
void leerBotones() {
  static unsigned long tRebote = 0;
  if (millis() - tRebote < 200) return;
  for (byte i = 0; i < 6; i++) {
    if (digitalRead(BOT_PLANTA[i]) == LOW) {
      plantaDestino = i;
      tRebote = millis();
      Serial.print(F("Llamada planta ")); Serial.println(i);
      return;
    }
  }
}

void leerMandoIR() {
  if (!IrReceiver.decode()) return;
  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
    IrReceiver.resume();
    return;
  }
  // Códigos NEC del mando wokwi-ir-remote (docs.wokwi.com).
  switch (IrReceiver.decodedIRData.command) {
    case 0x68: plantaDestino = 0; break;
    case 0x30: plantaDestino = 1; break;
    case 0x18: plantaDestino = 2; break;
    case 0x7A: plantaDestino = 3; break;
    case 0x10: plantaDestino = 4; break;
    case 0x38: plantaDestino = 5; break;
    default:   IrReceiver.resume(); return;
  }
  Serial.print(F("Mando IR -> planta ")); Serial.println(plantaDestino);
  IrReceiver.resume();
}

// ---------------- Movimiento del ascensor ----------------
void gestionarMovimiento() {
  if (plantaActual == plantaDestino) return;
  if (millis() - tPaso < T_PASO) return;
  tPaso = millis();
  plantaActual += (plantaDestino > plantaActual) ? 1 : -1;
  servoCabina.write(map(plantaActual, 0, 5, 0, 180));
  if (plantaActual == plantaDestino) {
    personas = (personas + 1) % 5;
    Serial.print(F("Llegada planta ")); Serial.print(plantaActual);
    Serial.print(F(" | personas=")); Serial.println(personas);
  }
}

// ---------------- Control discontinuo 3 posiciones con zona muerta ----------------
void controlTemperatura() {
  if (millis() - tCtrl < T_CTRL) return;
  tCtrl = millis();
  if      (tempMedida >= TEMP_SETPOINT + TEMP_ZM) { estadoEnf = true;  estadoCal = false; }
  else if (tempMedida <= TEMP_SETPOINT - TEMP_ZM) { estadoCal = true;  estadoEnf = false; }
  else                                            { estadoCal = false; estadoEnf = false; }
  digitalWrite(PIN_LED_CAL, estadoCal);
  digitalWrite(PIN_LED_ENF, estadoEnf);
}

// ---------------- Control de iluminación escalonado (8 etapas, 74HC595) ----------------
void controlIluminacion() {
  byte nuevos = (luzPct < LUZ_SETPOINT)
              ? (LUZ_SETPOINT - luzPct) / (LUZ_SETPOINT / LUZ_ETAPAS)
              : 0;
  if (nuevos == ledsLuz) return;
  ledsLuz = nuevos;
  escribir74HC((1 << ledsLuz) - 1);
}

void escribir74HC(byte valor) {
  digitalWrite(PIN_595_LATCH, LOW);
  shiftOut(PIN_595_DATA, PIN_595_CLK, MSBFIRST, valor);
  digitalWrite(PIN_595_LATCH, HIGH);
}

// ---------------- HMI: 3 vistas rotativas ----------------
void refrescarLCD() {
  if (millis() - tLcd < T_LCD) return;
  tLcd = millis();
  vistaLcd = (vistaLcd + 1) % 3;
  lcd.clear();
  switch (vistaLcd) {
    case 0: vistaAscensor();    break;
    case 1: vistaTemperatura(); break;
    case 2: vistaIluminacion(); break;
  }
}

void printPlanta(byte p) { if (p == 0) lcd.print('G'); else lcd.print(p); }

void vistaAscensor() {
  lcd.setCursor(0, 0);
  lcd.print(F("Planta:")); printPlanta(plantaActual);
  lcd.print(F("->"));      printPlanta(plantaDestino);
  lcd.print(plantaActual != plantaDestino ? F(" >>") : F(" =="));
  lcd.setCursor(0, 1);
  lcd.print(F("Personas:")); lcd.print(personas);
  lcd.print(F(" Hum:"));     lcd.print((int)humMedida); lcd.print('%');
}

void vistaTemperatura() {
  lcd.setCursor(0, 0);
  lcd.print(F("T:")); lcd.print(tempMedida, 1);
  lcd.write(0xDF);    lcd.print(F("C SP:")); lcd.print((int)TEMP_SETPOINT);
  lcd.setCursor(0, 1);
  if      (estadoCal) lcd.print(F("Accion: CALENTAR"));
  else if (estadoEnf) lcd.print(F("Accion: ENFRIAR "));
  else                lcd.print(F("Accion: ZMUERTA "));
}

void vistaIluminacion() {
  lcd.setCursor(0, 0);
  lcd.print(F("Luz:"));  lcd.print(luzPct);
  lcd.print(F("% SP:")); lcd.print(LUZ_SETPOINT); lcd.print('%');
  lcd.setCursor(0, 1);
  lcd.print(F("LEDs ON:")); lcd.print(ledsLuz);
  lcd.print('/');           lcd.print(LUZ_ETAPAS);
}
