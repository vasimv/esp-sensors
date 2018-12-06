// Internal ESP's sensors (ADC, button, led)

#include "internal.h"
#include "mqtt-support.h"

boolean ledState = false;

boolean FlagADC = false;

int buttonState;
int cntButton = 0;

// publish PNP information to MQTT
void pnp_internal() {
  pnp_mqtt(TOPIC_LED, "Built-in LED", "Builtin LEDs Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_BUTTON, "FLASH button", "Builtin Switches Buttons", "O:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_ADC, "ADC read", "Analogue ADC", "O:Number", "0", "1023");
  pnp_mqtt(TOPIC_ADC_CONTROL, "ADC on/off", "Builtin Switches ADC", "I:Switch", NULL, NULL);
} // void pnp_internal()

// subscribe to MQTT topics
void subscribe_internal() {
  subscribe_mqtt(TOPIC_ADC_CONTROL);
  subscribe_mqtt(TOPIC_LED);
} // void subscribe_internal()

// Setup hardware (pins, etc)
void setup_internal() {
  pinMode(BUTTON, INPUT);
  buttonState = digitalRead(BUTTON);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
} // void setup_internal()

// must be called from loop()
void loop_internal() {
  // Update LED
  digitalWrite(LED, ledState ? LOW : HIGH);

  // Check button
  if (buttonState != digitalRead(BUTTON)) {
    cntButton++;
    if (cntButton > 3) {
      buttonState = digitalRead(BUTTON);
      publish_mqttS(TOPIC_BUTTON, (char *) (buttonState ? "OFF" : "ON"));
    }
  } else
    cntButton = 0;
} // void loop_internal()

// last states (to prevent spamming)
boolean lastButtonState = false;
int lastADC = -1;

// periodic refreshing values to MQTT
void refresh_internal(boolean flagForce) {
  if (flagForce)
    publish_mqttS(TOPIC_BUTTON, (char *) (buttonState ? "OFF" : "ON"));

  if (FlagADC) {
    if (flagForce || (analogRead(A0) != lastADC)) {
      lastADC = analogRead(A0);
      publish_mqttI(TOPIC_ADC, lastADC);
    }
  }
} // void refresh_internal(boolean flagForce)

// hook for incoming MQTT messages
boolean mqtt_internal(char *topicCut, char *payload) {
  if (cmpTopic(topicCut, TOPIC_ADC_CONTROL)) {
    if (cmpPayloadON(payload))
      FlagADC = true;
    else
      FlagADC = false;
#ifdef DEBUG
    Serial.print("ADC switch update: ");
    Serial.println(FlagADC);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_LED)) {
    ledState = cmpPayloadON(payload);
    digitalWrite(LED, ledState ? LOW : HIGH);
#ifdef DEBUG
    Serial.print("LED update: ");
    Serial.println(ledState);
#endif
    return true;
  }
  return false;
} // boolean mqtt_internal(char *topicCut, char *payload)
