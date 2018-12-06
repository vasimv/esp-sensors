// Sonar's support (HC-SR04 and maxbotix with serial)

#ifndef _SONAR_H
#define _SONAR_H

#include "esp-sensors.h"

// publish PNP information to MQTT
void pnp_sonar();

// subscribe to MQTT topics
void subscribe_sonar();

// Setup hardware (pins, etc)
void setup_sonar();

// must be called from loop()
void loop_sonar();

// periodic refreshing values to MQTT
void refresh_sonar(boolean flagForce);

// hook for incoming MQTT messages
boolean mqtt_sonar(char *topicCut, char *payload);

#endif
