// Simple thermostat control (with two relays for fan and compressor units)

#ifndef _THERMOSTAT_H
#define _THERMOSTAT_H

#include "esp-sensors.h"
#include "weather.h"

// publish PNP information to MQTT
void pnp_thermostat();

// subscribe to MQTT topics
void subscribe_thermostat();

// Setup hardware (pins, etc)
void setup_thermostat();

// must be called from loop()
void loop_thermostat();

// periodic refreshing values to MQTT
void refresh_thermostat(boolean flagForce);

// hook for incoming MQTT messages
boolean mqtt_thermostat(char *topicCut, char *payload);

#endif
