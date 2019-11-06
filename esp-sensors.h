// Main configuration and includes

#ifndef _ESP_SENSORS_H
#define _ESP_SENSORS_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// What modules to use:
// HC-SR04 sonar (conflicts with MAXBOTIX_CONTROL!), sonar.cpp
// #define HCSR04_CONTROL

// Maxbotix serial sonar (conflicts with HCSR04_CONTROL and DIMMER_CONTROL!), sonar.cpp
// #define MAXBOTIX_CONTROL

// Modbus 4-ch dimmer (conflicts with MAXBOTIX_CONTROL!), dimmer.cpp
#define DIMMER_CONTROL

// Simple thermostat control (conflicts with MOVEMENT_CONTROL and SONOFF_CONTROL!), thermostat.cpp
// #define THERMOSTAT_CONTROL

// IR and X-band radio movement sensors (conflicts with THERMOSTAT_CONTROL and SONOFF_CONTROL!), moveir.cpp
#define MOVEMENT_CONTROL

// DHT22 weather sensor (must be included for THERMOSTAT_CONTROL, conflicts with GY39_CONTROL!), weather.cpp
#define DHT22_CONTROL

// GY-39 weather sensor over serial port (conflicts with DHT22_CONTROL!), weather.cpp
// #define GY39_CONTROL

// Sonoff basic and Sonoff touch (conflicts with MOVEMENT_CONTROL!), switch.cpp
// #define SONOFF_CONTROL


// Configuration stuff:
// Debug output to serial port
// #define DEBUG

// How often to call refresh_* functions, milliseconds
#define REFRESH_TIME 200

// How often to call refresh_* functions with flagForce, milliseconds
#define FORCE_TIME 2000

// Disable internal led blinking at connecting
//#define DISABE_BLINK

// Configuration Wifi network password
#define WIFI_CONFIGURE_PASS "passtest"

// MQTT username and password
#define MQTT_USER_NAME "testuser"
#define MQTT_PASSWORD "testtest"
#define MQTT_PORT 1883

// MQTT Prefixes
#define MQTT_PREFIX "/myhome/ESPX-"
#define MQTT_NAME "espx-"
#define MQTT_PNP_PREFIX "/myhome/PNP"

// OTA firmware upload password
#define OTA_UPLOAD_PASS "Bi38s2iw"

// Dimmer's PWM maximum
#define LED_PWMRANGE 1023

// Hystersis of thermostat regulation (in celsius)
#define THERMOSTAT_HYSTERSIS 1.0f


// Pins definition
// HC-SR04 sonar pins
#define HCSR04_ECHO 4
#define HCSR04_TRIG 15

// Serial port pins (for dimmer or maxbotix)
#define rxPin 16
#define txPin 4
#define RS485_DE_PIN 15

// Thermostat pins
#define FAN_RELAY 4
#define COMPRESSOR_RELAY 15
#define LED_OFF 14
#define LED_ON 12
#define LED_GND 13

// DHT22 pin
#define DHTPIN 5

// GY39 pin
#define GY39PIN 5

// Internal LED and button pins
#define LED 2
#define BUTTON 0

// Movement sensors pins
#define IR_SENSE 14
#define RADIO_SENSE 12
#define ADD_SENSE 13

// Sonoff basic and Sonoff touch AC relay pin
#define RELAY 12


// MQTT Topics names
// esp-sensors.cpp
#define TOPIC_REBOOT "REBOOT"
#define TOPIC_RECONNECT "RECONNECT"

// internal.cpp
#define TOPIC_BUTTON "BUTTON"
#define TOPIC_LED "LED"
#define TOPIC_ADC "ADC"
#define TOPIC_ADC_CONTROL "ADCSWITCH"

// moveir.cpp
#define TOPIC_IR_SENSE "MOVEIR"
#define TOPIC_RADIO_SENSE "MOVERADIO"
#define TOPIC_ADDITIONAL "MOVEADD"

// sonar.cpp
#define TOPIC_DISTANCE "DISTANCE"

// dimmer.cpp
#define TOPIC_R_LED "LED_RED"
#define TOPIC_G_LED "LED_GREEN"
#define TOPIC_B_LED "LED_BLUE"
#define TOPIC_W_LED "LED_WHITE"
#define TOPIC_GRADUALLY "GRADUALLY"

// weather.cpp
#define TOPIC_DHT "DHT"
#define TOPIC_TEMPERATURE "TEMPERATURE"
#define TOPIC_HUMIDITY "HUMIDITY"
#define TOPIC_GY39 "GY39"
#define TOPIC_ILLUMINATION "ILLUMINATION"
#define TOPIC_PRESSURE "PRESSURE"

// switch.cpp
#define TOPIC_RELAY "RELAY"
#define TOPIC_AUTO_RELAY "AUTOSWITCH"

// thermostat.cpp
#define TOPIC_FAN_AUTO "FAN_AUTO"
#define TOPIC_FAN "FAN"
#define TOPIC_FAN_MANUAL "FAN_MANUAL"
#define TOPIC_COMPRESSOR_AUTO "COMPRESSOR_AUTO"
#define TOPIC_COMPRESSOR_STATUS "COMPRESSOR_STATUS"
#define TOPIC_COMPRESSOR "COMPRESSOR"
#define TOPIC_TEMP_AUTO "AUTO_TEMP"
#define TOPIC_LED_ON "LED_ON"
#define TOPIC_LED_OFF "LED_OFF"


extern WiFiClient wifi;
extern PubSubClient myClient;

#endif
