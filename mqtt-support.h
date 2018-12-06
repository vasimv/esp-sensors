// Common MQTT stuff

#ifndef _MQTT_SUPPORT_H
#define _MQTT_SUPPORT_H

#include "esp-sensors.h"
#include <PubSubClient.h>

// MQTT prefix name (like "/myhome/ESPX-0003DCE2")
extern char *mqttPrefixName;

// Connect to MQTT broker, returns true if connected
boolean connect_mqtt(char *mqttServer, char *mqttClientName, char *mqttUser, char *mqttPass, std::function<void(char*, uint8_t*, unsigned int)> mqttCallback);

// Check connection to MQTT broker
boolean check_mqtt();

// Internal mqtt stuff
void mqtt_loop();

// Publish PNP info to MQTT
void pnp_mqtt(char *topic, char *name, char *groups, char *type, char *min, char *max);

// Subscribe to MQTT topics
void subscribe_mqtt(char *topic);

// Publish to topic (int)
void publish_mqttI(char *topic, int value);

// Publish to topic (string)
void publish_mqttS(char *topic, char *value);

// Publish to topic (float)
void publish_mqttF(char *topic, float value);

// Compare string to topic name
boolean cmpTopic(char *incoming, char *topic);

// Compare message for "ON"
boolean cmpPayloadON(char *msg);

// Compare message for "OFF"
boolean cmpPayloadOFF(char *msg);

#endif
