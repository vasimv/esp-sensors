// "Sonoff switch" and "Sonoff touch" support

#include "switch.h"
#include "mqtt-support.h"

// publish PNP information to MQTT
void pnp_switch() {
  pnp_mqtt(TOPIC_RELAY, "AC relay", "Relays Switches AC", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_AUTO_RELAY, "Auto relay on/off", "Switches", "I:Switch", NULL, NULL);
} // void pnp_switch()

// subscribe to MQTT topics
void subscribe_switch() {
  subscribe_mqtt(TOPIC_RELAY);
  subscribe_mqtt(TOPIC_AUTO_RELAY);
} // void subscribe_switch()

boolean FlagRelay = true;
boolean relayState = false;

int touchState;
int cntTouch = 0;

// Setup hardware (pins, etc)
void setup_switch() {
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, relayState ? HIGH : LOW);
  touchState = digitalRead(BUTTON);
} // void setup_switch()

// must be called from loop()
void loop_switch() {
  // Check touch button
  if (touchState != digitalRead(BUTTON)) {
    cntTouch++;
    if (cntTouch > 3) {
      touchState = digitalRead(BUTTON);
      if (touchState == LOW) {
        // Light up LED while button is pressed
        analogWrite(LED, 512);
        // Auto switch relay
        if (FlagRelay) {
          relayState = !relayState;
          digitalWrite(RELAY, relayState ? HIGH : LOW);
          publish_mqttS(TOPIC_RELAY, (char *) (relayState ? "ON" : "OFF"));
        }
      } else
        digitalWrite(LED, HIGH);
    }
  } else
    cntTouch = 0;
} // void loop_switch()

// periodic refreshing values to MQTT
void refresh_switch(boolean flagForce) {
  if (flagForce)
    publish_mqttS(TOPIC_RELAY, (char *) (relayState ? "ON" : "OFF"));
} // void refresh_switch(boolean flagForce)

// hook for incoming MQTT messages
boolean mqtt_switch(char *topicCut, char *payload) {
  if (cmpTopic(topicCut, TOPIC_RELAY)) {
    if (cmpPayloadON(payload)) {
      digitalWrite(RELAY, HIGH);
      relayState = true;
    } else {
      digitalWrite(RELAY, LOW);
      relayState = false;
    }
#ifdef DEBUG
    Serial.print("Relay status update: ");
    Serial.println(relayState);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_AUTO_RELAY)) {
    if (cmpPayloadON(payload))
      FlagRelay = true;
    else
      FlagRelay = false;
#ifdef DEBUG
    Serial.print("Auto relay switch update: ");
    Serial.println(FlagRelay);
#endif
    return true;
  }
  return false;
} // boolean mqtt_switch(char *topicCut, char *payload)
