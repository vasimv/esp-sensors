// 4ch dimmer

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "dimmer.h"
#include "mqtt-support.h"

// publish PNP information to MQTT
void pnp_dimmer() {
  pnp_mqtt(TOPIC_R_LED, "Red LED", "Dimmers LEDs Light", "I:Dimmer", NULL, NULL);
  pnp_mqtt(TOPIC_G_LED, "Green LED", "Dimmers LEDs Light", "I:Dimmer", NULL, NULL);
  pnp_mqtt(TOPIC_B_LED, "Blue LED", "Dimmers LEDs Light", "I:Dimmer", NULL, NULL);
  pnp_mqtt(TOPIC_W_LED, "White LED", "Dimmers LEDs Light", "I:Dimmer", NULL, NULL);
  pnp_mqtt(TOPIC_GRADUALLY, "Gradually on/off", "LEDs Switches", "I:Switch", NULL, NULL);
} // void pnp_dimmer()

// subscribe to MQTT topics
void subscribe_dimmer() {
  subscribe_mqtt(TOPIC_R_LED);
  subscribe_mqtt(TOPIC_G_LED);
  subscribe_mqtt(TOPIC_B_LED);
  subscribe_mqtt(TOPIC_W_LED);
  subscribe_mqtt(TOPIC_GRADUALLY);
} // void subscribe_dimmer()

// LEDs state
int16_t cLED[4];
// LEDs target luminiosity
int16_t tLED[4];

int8_t FlagChangeLED = 0;
uint8_t FlagGradually = 0;

unsigned long startEffects = 0;
unsigned long lastChangeEffect = 0;
uint8_t stepEffect = 0;

// set up a new serial port
SoftwareSerial mySerial(rxPin, txPin, false, 64);

// Set LEDs according ALL led status (with effects)
void SetAllLED() {
  int i;
  int diff;

  if (!FlagGradually) {
    // Set luminosity immediately
    for (i = 0; i < 4; i++) {
      if (cLED[i] != tLED[i]) {
        cLED[i] = tLED[i];
        FlagChangeLED = 2;
      }
    }
  } else {
    // Set luminosity gradually
    for (i = 0; i < 4; i++) {
      if (cLED[i] != tLED[i]) {
        diff = tLED[i] - cLED[i];
        if (diff > 5)
          diff = 5;
        if (diff < -5)
          diff = -5;
        cLED[i] += diff;
        FlagChangeLED = 2;
      }
    }
  }
} // void SetAllLED()

uint8_t bufModbusOut[128];
uint16_t cntOut = 0;

// Calculate CRC for modbus frame
uint16_t ModbusCrc(uint8_t *buf, int len) {
  uint32_t tmp, tmp2;
  uint8_t Flag;
  uint16_t i, j;

  tmp = 0xFFFF;
  for (i = 0; i < len; i++) {
    tmp = tmp ^ buf[i];
    for (j = 1; j <= 8; j++) {
      Flag = tmp & 0x0001;
      tmp >>= 1;
      if (Flag)
        tmp ^= 0xA001;
    }
  }
  tmp2 = tmp >> 8;
  tmp = (tmp << 8) | tmp2;
  tmp &= 0xFFFF;
  return (uint16_t) tmp;
} // uint16_t ModbusCrc(char *buf, int len)

// Add CRC to output modbus packet
void AddModbusCrc() {
  uint16_t tCrc;

  tCrc = ModbusCrc(bufModbusOut, cntOut);
  bufModbusOut[cntOut] = (tCrc >> 8) & 0xff;
  cntOut++;
  bufModbusOut[cntOut] = tCrc & 0xff;
  cntOut++;
} // void AddModbusCrc()
unsigned long lastRepeat = 0;

// Send LEDs statuses to the dimmer by Modbus
void SendLED() {
  int i;

  if (FlagChangeLED > 0) {
    if ((FlagChangeLED == 1) && ((millis() - lastRepeat) < 100))
      return;
    bufModbusOut[0] = 247;
    bufModbusOut[1] = 16;
    bufModbusOut[2] = 0;
    bufModbusOut[3] = 0;
    bufModbusOut[4] = 0;
    bufModbusOut[5] = 4;
    bufModbusOut[6] = 8;
    for (i = 0; i < 4; i++) {
#ifdef DEBUG
      Serial.print(i);
      Serial.print(':');
      Serial.println(cLED[i]);
#endif
      bufModbusOut[i * 2 + 7] = (cLED[i] >> 8) & 0xff;
      bufModbusOut[i * 2 + 8] = cLED[i] & 0xff;
    }
    lastRepeat = millis();
    cntOut = 15;
    AddModbusCrc();
#ifdef DEBUG
    for (i = 0; i < cntOut; i++) {
      Serial.println(bufModbusOut[i]);
    }
#endif
    digitalWrite(RS485_DE_PIN, HIGH);
    delay(1);
    mySerial.write(bufModbusOut, cntOut);
    digitalWrite(RS485_DE_PIN, LOW);
#ifdef DEBUG
    Serial.write(bufModbusOut, cntOut);
#endif
    FlagChangeLED--;
    if (FlagChangeLED < 0)
      FlagChangeLED = 0;
  }
} // void SendLED()

// Setup hardware (pins, etc)
void setup_dimmer() {
#ifndef DIMMER_SIMPLE_LEDS
  pinMode(RS485_DE_PIN, OUTPUT);
  digitalWrite(RS485_DE_PIN, LOW);
  mySerial.begin(38400);
#else
  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, LOW);
  pinMode(LED_BLUE_PIN, OUTPUT);
  digitalWrite(LED_BLUE_PIN, LOW);
  pinMode(LED_GREEN_PIN, OUTPUT);
  digitalWrite(LED_GREEN_PIN, LOW);
  pinMode(LED_WHITE_PIN, OUTPUT);
  digitalWrite(LED_WHITE_PIN, LOW);
#endif

  memset((void *) cLED, 0, sizeof(cLED));
  memset((void *) tLED, 0, sizeof(cLED));
  FlagChangeLED = 2;
} // void setup_dimmer()

// must be called from loop()
void loop_dimmer() {
  SetAllLED();
  // Repeat dimmer set frame every second (just in case)
  if (!FlagChangeLED && (millis() - lastRepeat) > 1000)
    FlagChangeLED = 1;
#ifndef DIMMER_SIMPLE_LEDS
  SendLED();
#else
  analogWrite(LED_RED_PIN, cLED[0]);
  analogWrite(LED_GREEN_PIN, cLED[1]);
  analogWrite(LED_BLUE_PIN, cLED[2]);
  analogWrite(LED_WHITE_PIN, cLED[3]);
#endif
} // void loop_dimmer()

// periodic refreshing values to MQTT
void refresh_dimmer(boolean flagForce) {
} // void refresh_dimmer(boolean flagForce)

// Check message for ON/OFF or number in percents
int dimmerLight(char *payload) {
  if (cmpPayloadON(payload))
    return 100;
  if (cmpPayloadOFF(payload))
    return 0;
  return strtod(payload, NULL);
} // int dimmerLight(char *payload)

// Check case when we're turning off lights with gradually on
void checkGradually(int lightNum) {
  int differ = cLED[lightNum] - tLED[lightNum];

  // Not gradually or turning on - just set choosed level
  if (!FlagGradually || (differ <= 0))
    return;
  // Gradually OFF - drop level to 50% at first!
  cLED[lightNum] = cLED[lightNum] - differ / 2; 
} // void checkGradually(int lightNum)

// hook for incoming MQTT messages
boolean mqtt_dimmer(char *topicCut, char *payload) {
  if (cmpTopic(topicCut, TOPIC_R_LED)) {
    tLED[0] = (dimmerLight(payload) * LED_PWMRANGE) / 100;
    checkGradually(0);
#ifdef DEBUG
    Serial.print("Red color: ");
    Serial.println(tLED[0]);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_G_LED)) {
    tLED[1] = (dimmerLight(payload) * LED_PWMRANGE) / 100;
    checkGradually(1);
#ifdef DEBUG
    Serial.print("Green color: ");
    Serial.println(tLED[1]);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_B_LED)) {
    tLED[2] = (dimmerLight(payload) * LED_PWMRANGE) / 100;
    checkGradually(2);
#ifdef DEBUG
    Serial.print("Blue color: ");
    Serial.println(tLED[2]);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_W_LED)) {
    tLED[3] = (dimmerLight(payload) * LED_PWMRANGE) / 100;
    checkGradually(3);
#ifdef DEBUG
    Serial.print("White color: ");
    Serial.println(tLED[3]);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_GRADUALLY)) {
    if (cmpPayloadON(payload))
      FlagGradually = 1;
    else
      FlagGradually = 0;
    return true;
  }
  return false;
} // boolean mqtt_dimmer(char *topicCut, char *payload)
