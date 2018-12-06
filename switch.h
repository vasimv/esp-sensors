// Sonoff basic switch and Sonoff touch support

#ifndef _SWITCH_H
#define _SWITCH_H

#include "esp-sensors.h"

// publish PNP information to MQTT
void pnp_switch();

// subscribe to MQTT topics
void subscribe_switch();

// Setup hardware (pins, etc)
void setup_switch();

// must be called from loop()
void loop_switch();

// periodic refreshing values to MQTT
void refresh_switch(boolean flagForce);

// hook for incoming MQTT messages
boolean mqtt_switch(char *topicCut, char *payload);

#endif
