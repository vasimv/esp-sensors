// Movement sensors (IR and X-band radio)

#include "moveir.h"
#include "mqtt-support.h"

// publish PNP information to MQTT
void pnp_moveir() {
  pnp_mqtt(TOPIC_IR_SENSE, "IR sensor", "Sensors Movement IR", "O:Number", "0", "100");
  pnp_mqtt(TOPIC_RADIO_SENSE, "Radio sensor", "Sensors Movement Radio", "O:Number", "0", "100");
  pnp_mqtt(TOPIC_ADDITIONAL, "Additional IR sensor", "Sensors Movement IR", "O:Number", "0", "100");
} // void pnp_moveir()

// subscribe to MQTT topics
void subscribe_moveir() {
} // void subscribe_moveir()

// Vars
int FlagIrSense = 0;
int FlagRadioSense = 0;
int FlagAddSense = 0;

int lastIrSense = 0;
int lastRadioSense = 0;
int lastAddSense = 0;

// Last states (to not report same value too often)
int lastRadioSenseState = -1;
int lastIrSenseState = -1;
int lastAddIrSenseState = -1;

// Setup hardware (pins, etc)
void setup_moveir() {
  pinMode(IR_SENSE, INPUT);
  pinMode(RADIO_SENSE, INPUT);
  pinMode(ADD_SENSE, INPUT);
} // void setup_moveir()

// must be called from loop()
void loop_moveir() {
  unsigned long startDelay = millis();

  // Check sensors for 9ms
  while ((millis() - startDelay) < 9) {
   if (digitalRead(IR_SENSE) != lastIrSense) {
      FlagIrSense++;
      lastIrSense = digitalRead(IR_SENSE);
    }
    if (digitalRead(RADIO_SENSE) != lastRadioSense) {
      FlagRadioSense++;
      lastRadioSense = digitalRead(RADIO_SENSE);
    }
    if (digitalRead(ADD_SENSE) != lastAddSense) {
      FlagAddSense++;
      lastAddSense = digitalRead(ADD_SENSE);
    }
  }
} // void loop_moveir()

// periodic refreshing values to MQTT
void refresh_moveir(boolean flagForce) {
  int tmp;
  
  // Publish statuses of IR/RADIO sensors
  tmp = (FlagIrSense > 100) ? 100 : FlagIrSense;
  if (flagForce || (lastIrSenseState != tmp)) {
#ifdef DEBUG
    Serial.println("Reporting changed IR status");
#endif
    lastIrSenseState = tmp;
    publish_mqttI(TOPIC_IR_SENSE, tmp);
  }
  tmp = (FlagRadioSense > 100) ? 100 : FlagRadioSense;
  if (flagForce || (lastRadioSenseState != tmp)) {
#ifdef DEBUG
    Serial.println("Reporting changed RADIO status");
#endif
    lastRadioSenseState = tmp;
    publish_mqttI(TOPIC_RADIO_SENSE, tmp);
  }
  tmp = (FlagAddSense > 100) ? 100 : FlagAddSense;
  if (flagForce || (lastAddIrSenseState != tmp)) {
#ifdef DEBUG
    Serial.println("Reporting changed ADDIR status");
#endif
    lastAddIrSenseState = tmp;
    publish_mqttI(TOPIC_ADDITIONAL, tmp);
  }

  FlagRadioSense = 0;
  FlagIrSense = 0;
  FlagAddSense = 0;
} // void refresh_moveir(boolean flagForce)

// hook for incoming MQTT messages
boolean mqtt_moveir(char *topicCut, char *payload) {
  return false;
} // boolean mqtt_moveir(char *topicCut, char *payload)
