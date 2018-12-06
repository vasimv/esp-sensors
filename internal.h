// ESP's internal sensors (ADC, button, led)

#ifndef _INTERNAL_H
#define _INTERNAL_H

#include "esp-sensors.h"

// LED control
extern boolean ledState;

// publish PNP information to MQTT
void pnp_internal();

// subscribe to MQTT topics
void subscribe_internal();

// Setup hardware (pins, etc)
void setup_internal();

// must be called from loop()
void loop_internal();

// periodic refreshing values to MQTT
void refresh_internal(boolean flagForce);

// hook for incoming MQTT messages
boolean mqtt_internal(char *topicCut, char *payload);

#endif
