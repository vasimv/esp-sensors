// Movement sensors (IR and X-band radio)

#ifndef _MOVEIR_H
#define _MOVEIR_H

#include "esp-sensors.h"


// publish PNP information to MQTT
void pnp_moveir();

// subscribe to MQTT topics
void subscribe_moveir();

// Setup hardware (pins, etc)
void setup_moveir();

// must be called from loop()
void loop_moveir();

// periodic refreshing values to MQTT
void refresh_moveir(boolean flagForce);

// hook for incoming MQTT messages
boolean mqtt_moveir(char *topicCut, char *payload);

#endif
