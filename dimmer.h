// 4ch dimmer

#ifndef _DIMMER_H
#define _DIMMER_H

#include "esp-sensors.h"


// publish PNP information to MQTT
void pnp_dimmer();

// subscribe to MQTT topics
void subscribe_dimmer();

// Setup hardware (pins, etc)
void setup_dimmer();

// must be called from loop()
void loop_dimmer();

// periodic refreshing values to MQTT
void refresh_dimmer(boolean flagForce);

// hook for incoming MQTT messages
boolean mqtt_dimmer(char *topicCut, char *payload);

#endif
