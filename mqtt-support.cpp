// MQTT support routines

#include "esp-sensors.h"
#include "mqtt-support.h"

// Prefix for MQTT topics
char *mqttPrefixName;

// Check connection to MQTT broker
boolean check_mqtt() {
  if (myClient.connected())
    return true;
  return false;
} // boolean check_mqtt()

// Internal mqtt stuff
void mqtt_loop() {
  myClient.loop();
} // void mqtt_loop()

char tmpMqttTopic[256];

// Use unique MQTT prefix to generate topic name (be sure outBuffer has enough space - +32 bytes!!!)
void generateMqttTopic(char *outBuffer, char *topic, char *subtopic) {
  sprintf(outBuffer, "%s%s/%s", mqttPrefixName, topic, subtopic);
} // void generateMqttTopic(char *outbuffer, char *topic, char *subtopic)

// Connect to MQTT broker, returns true if connected
boolean connect_mqtt(char *mqttServer, char *mqttClientName, char *mqttUser, char *mqttPass, std::function<void(char*, uint8_t*, unsigned int)> mqttCallback) {
  if (myClient.connected())
    return true;
  myClient.setServer(mqttServer, MQTT_PORT);
  myClient.setCallback(mqttCallback);
  if (myClient.connect(mqttClientName, mqttUser, mqttPass)) {
#ifdef DEBUG
    Serial.println("Connected to MQTT broker");
#endif
    return true;
  } else {
#ifdef DEBUG
    Serial.println("Failed to connect to MQTT broker!");
#endif
    return false;
  }
} // boolean connect_mqtt(char *mqttServer, char *mqttClientName, char *mqttUser, char *mqttPass, std::function<void(char*, uint8_t*, unsigned int)> mqttCallback)

char tmpBuf[256];

// Publish Plug&Play info to MQTT
void pnp_mqtt(char *topic, char *name, char *groups, char *type, char *min, char *max) {
  if (!myClient.connected())
    return;
  snprintf(tmpBuf, sizeof(tmpBuf) - 1, "%s%s%s/%s", MQTT_PNP_PREFIX, mqttPrefixName, topic, "name");
  myClient.publish(tmpBuf, name, true);
#ifdef DEBUG
  Serial.println("PnP: ");
  Serial.print(tmpBuf);
  Serial.print(':');
  Serial.print(name);
#endif
  snprintf(tmpBuf, sizeof(tmpBuf) - 1, "%s%s%s/%s", MQTT_PNP_PREFIX, mqttPrefixName, topic, "groups");
  myClient.publish(tmpBuf, groups, true);
#ifdef DEBUG
  Serial.println("PnP: ");
  Serial.print(tmpBuf);
  Serial.print(':');
  Serial.print(groups);
#endif
  snprintf(tmpBuf, sizeof(tmpBuf) - 1, "%s%s%s/%s", MQTT_PNP_PREFIX, mqttPrefixName, topic, "type");
  myClient.publish(tmpBuf, type, true);
#ifdef DEBUG
  Serial.println("PnP: ");
  Serial.print(tmpBuf);
  Serial.print(':');
  Serial.print(type);
#endif
  if (min != NULL) {
    snprintf(tmpBuf, sizeof(tmpBuf) - 1, "%s%s%s/%s", MQTT_PNP_PREFIX, mqttPrefixName, topic, "min");
    myClient.publish(tmpBuf, min, true);
  }
  if (max != NULL) {
    snprintf(tmpBuf, sizeof(tmpBuf) - 1, "%s%s%s/%s", MQTT_PNP_PREFIX, mqttPrefixName, topic, "max");
    myClient.publish(tmpBuf, max, true);
  }
  delay(10);
} // void pnp_mqtt(char *topic, char *name, char *groups, char *type, char *min, char *max) 

// Subscribe to MQTT topics
void subscribe_mqtt(char *topic) {
  if (!myClient.connected())
    return;
  generateMqttTopic(tmpMqttTopic, topic, "status");
  myClient.subscribe(tmpMqttTopic);
  myClient.loop();
#ifdef DEBUG
  Serial.print("Subscribe: ");
  Serial.println(tmpMqttTopic);
#endif
  delay(10);
} // void subscribe_mqtt(char *topic)

char tmpValue[64];

// Publish to topic (int)
void publish_mqttI(char *topic, int value) {
  sprintf(tmpValue, "%d", value);
  publish_mqttS(topic, tmpValue);
} // void publish_mqttI(char *topic, int value)

// Publish to topic (string)
void publish_mqttS(char *topic, char *value) {
  if (!myClient.connected())
    return;
  generateMqttTopic(tmpMqttTopic, topic, "status");
  myClient.publish(tmpMqttTopic, value, true);
#ifdef DEBUG
  Serial.print(tmpMqttTopic);
  Serial.print('=');
  Serial.println(value);
#endif
} // void publish_mqttS(char *topic, char *value)

// Publish to topic (float)
void publish_mqttF(char *topic, float value) {
  char *p;
  char floatBuf[16];

  p = dtostrf(value, 5, 2, floatBuf);
  // Cut leading space
  while (*p == ' ')
    p++;
  publish_mqttS(topic, p);
} // void publish_mqttF(char *topic, float value)

// Compare message for "ON"
boolean cmpPayloadON(char *msg) {
  if ((msg[1] == 'N') || (msg[1] == 'n'))
    return true;
  return false;
} // boolean cmpPayloadON(char *msg)

// Compare message for "OFF"
boolean cmpPayloadOFF(char *msg) {
  if ((msg[1] == 'F') || (msg[1] == 'f'))
    return true;
  return false;
} // boolean cmpPayloadOFF(char *msg)

// Compare string to topic name
boolean cmpTopic(char *incoming, char *topic) {
  int topicLength = strlen(topic);

  if (memcmp(incoming, topic, topicLength))
    return false;
  if ((incoming[topicLength] != '/') || (incoming[topicLength + 1] == '\0'))
    return false;
  if (!strcmp(incoming + topicLength + 1, "status"))
    return true;
  return false;
} // boolean cmpTopic(char *incoming, char *topic)
