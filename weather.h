// DHT22 weather sensor support

#ifndef _WEATHER_H
#define _WEATHER_H

#include "esp-sensors.h"

// publish PNP information to MQTT
void pnp_weather();

// subscribe to MQTT topics
void subscribe_weather();

// Setup hardware (pins, etc)
void setup_weather();

// must be called from loop()
void loop_weather();

// periodic refreshing values to MQTT
void refresh_weather(boolean flagForce);

// hook for incoming MQTT messages
boolean mqtt_weather(char *topicCut, char *payload);

// Last temperature read
extern float temperatureCurrent;

// Last humidity read
extern float humidityCurrent;

// Last pressure read (only for GY-39)
extern float pressureCurrent;

// Last illumination read (only for GY-39)
extern float illuminationCurrent;

// Check/report DHT22 or GY-39 readings flag
extern boolean FlagReportWeather;

#endif
