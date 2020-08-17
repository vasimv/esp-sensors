// Simple thermostat control (two relays control fan and compressor units)

#include "thermostat.h"
#include "weather.h"
#include "mqtt-support.h"

boolean fanAuto = false;
boolean fanOverride = false;
boolean compressorAuto = false;
float tempMin = 25.0f;
float tempMax = 26.0f;

// publish PNP information to MQTT
void pnp_thermostat() {
  pnp_mqtt(TOPIC_LED_OFF, "Off LED", "Thermostat LEDs Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_LED_ON, "On LED", "Thermostat LEDs Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_FAN_AUTO, "Enable fan always when auto-control is on", "Thermostat Fan Climate Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_FAN, "Fan status", "Thermostat Fan Climate Switches", "O:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_FAN_MANUAL, "Fan manual control", "Thermostat Fan Climate Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_COMPRESSOR_AUTO, "Cooling auto control", "Thermostat Cooling Climate Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_COMPRESSOR, "Cooling unit relay status", "Thermostat Compressor Cooling Climate Switches", "O:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_COMPRESSOR_STATUS, "Cooling auto status", "Thermostat Compressor Cooling Climate Switches", "O:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_TEMP_AUTO, "Thermostat temperature", "Thermostat Temperature Climate", "I:Number", "18", "35");
} // void pnp_thermostat()

// subscribe to MQTT topics
void subscribe_thermostat() {
  subscribe_mqtt(TOPIC_LED_OFF);
  subscribe_mqtt(TOPIC_LED_ON);
  subscribe_mqtt(TOPIC_FAN_AUTO);
  subscribe_mqtt(TOPIC_FAN_MANUAL);
  subscribe_mqtt(TOPIC_COMPRESSOR_AUTO);
  subscribe_mqtt(TOPIC_TEMP_AUTO);
} // void subscribe_thermostat()

// Setup hardware (pins, etc)
void setup_thermostat() {
  pinMode(FAN_RELAY, OUTPUT);
  digitalWrite(FAN_RELAY, LOW);
  pinMode(COMPRESSOR_RELAY, OUTPUT);
  digitalWrite(COMPRESSOR_RELAY, LOW);

  pinMode(LED_OFF, OUTPUT);
  digitalWrite(LED_OFF, LOW);
  pinMode(LED_ON, OUTPUT);
  digitalWrite(LED_ON, LOW);

  pinMode(LED_GND, OUTPUT);
  digitalWrite(LED_GND, LOW);
} // void setup_thermostat()

unsigned long lastButtonPress = 0;
unsigned long lastThermostat = 0;

// must be called from loop()
void loop_thermostat() {
  // We turn on DHT22 always
  FlagReportWeather = true;

  // We process button press without MQTT in thermostat mode
  if ((digitalRead(BUTTON) == LOW) && ((millis() - lastButtonPress) > 1000)) {
    lastButtonPress = millis();
    if (compressorAuto) {
      compressorAuto = false;
      digitalWrite(LED_OFF, HIGH);
      digitalWrite(FAN_RELAY, LOW);
      digitalWrite(COMPRESSOR_RELAY, LOW);
    } else {
      compressorAuto = true;
      digitalWrite(LED_ON, HIGH);
    }
    publish_mqttS(TOPIC_COMPRESSOR_STATUS, (char *) (compressorAuto ? "ON" : "OFF"));

    delay(1000);
    digitalWrite(LED_OFF, LOW);
    digitalWrite(LED_ON, LOW);
  }

  // Control thermostat check (every 300 ms)
  if ((millis() - lastThermostat) > 300) {
    lastThermostat = millis();
    if (compressorAuto) {
      // Thermostat control
      if (tempMax <= temperatureCurrent) {
        // We always turn on fan together with compressor unit to prevent ice
        digitalWrite(FAN_RELAY, HIGH);
        digitalWrite(COMPRESSOR_RELAY, HIGH);
#ifdef DEBUG
        Serial.println("Temperature is bigger than maximum, turning on fan/compressor");
#endif
      }
      if (tempMin >= temperatureCurrent) {
        // We don't turn off FAN if fan isn't in auto mode or manually turned on
        if (!fanAuto && !fanOverride)
          digitalWrite(FAN_RELAY, LOW);
        digitalWrite(COMPRESSOR_RELAY, LOW);
#ifdef DEBUG
        Serial.println("Temperature is lower than minimum, turning off fan/compressor");
#endif
      }
#ifdef DEBUG
      Serial.print("FAN/COMPRESSOR status: ");
      Serial.print(digitalRead(FAN_RELAY));
      Serial.print(" ");
      Serial.println(digitalRead(COMPRESSOR_RELAY));
#endif
    } else
      if (!fanOverride)
        digitalWrite(FAN_RELAY, LOW);
  }
} // void loop_thermostat()

// periodic refreshing values to MQTT
void refresh_thermostat(boolean flagForce) {
  if (flagForce) {
    publish_mqttS(TOPIC_COMPRESSOR, (char *) ((digitalRead(COMPRESSOR_RELAY) == HIGH) ? "ON" : "OFF"));
    publish_mqttS(TOPIC_FAN, (char *) ((digitalRead(FAN_RELAY) == HIGH) ? "ON" : "OFF"));
    publish_mqttS(TOPIC_FAN_AUTO, (char *) (fanAuto ? "ON" : "OFF"), true);
    publish_mqttS(TOPIC_COMPRESSOR_AUTO, (char *) (compressorAuto ? "ON" : "OFF"), true);
    publish_mqttS(TOPIC_FAN_MANUAL, (char *) (fanOverride ? "ON" : "OFF"), true);
    publish_mqttS(TOPIC_LED_OFF, (char *) ((digitalRead(LED_OFF) == HIGH) ? "ON" : "OFF"), true);
    publish_mqttS(TOPIC_LED_ON, (char *) ((digitalRead(LED_ON) == HIGH) ? "ON" : "OFF"), true);
  }
} // void refresh_thermostat()

// hook for incoming MQTT messages
boolean mqtt_thermostat(char *topicCut, char *payload) {
  if (cmpTopic(topicCut, TOPIC_FAN_AUTO)) {
    if (cmpPayloadON(payload))
      fanAuto = true;
    else {
      fanAuto = false;
      // Turn off compressor and fan if needed
      if (!compressorAuto)
        digitalWrite(COMPRESSOR_RELAY, LOW);
      if (!fanOverride && !compressorAuto)
        digitalWrite(FAN_RELAY, LOW);
    }
#ifdef DEBUG
    Serial.print("Fan auto control: ");
    Serial.println(fanAuto);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_COMPRESSOR_AUTO)) {
    if (cmpPayloadON(payload))
      compressorAuto = true;
    else {
      compressorAuto = false;
      // Turn off compressor and fan
      digitalWrite(COMPRESSOR_RELAY, LOW);
      if (!fanOverride)
        digitalWrite(FAN_RELAY, LOW);
    }
#ifdef DEBUG
    Serial.print("Compressor auto control: ");
    Serial.println(compressorAuto);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_FAN_MANUAL)) {
    if (cmpPayloadON(payload)) {
      fanOverride = true;
      digitalWrite(FAN_RELAY, HIGH);
    } else {
      fanOverride = false;
      if (!compressorAuto)
        digitalWrite(FAN_RELAY, LOW);
    }
#ifdef DEBUG
    Serial.print("FAN manual switch: ");
    Serial.println(fanOverride);
#endif
    return true;
  } else if (cmpTopic(topicCut, TOPIC_LED_OFF)) {
    if (cmpPayloadON(payload))
      digitalWrite(LED_OFF, HIGH);
    else
      digitalWrite(LED_OFF, LOW);
    return true;
  } else if (cmpTopic(topicCut, TOPIC_LED_ON)) {
    if (cmpPayloadON(payload))
      digitalWrite(LED_ON, HIGH);
    else
      digitalWrite(LED_ON, LOW);
    return true;
  } if (cmpTopic(topicCut, TOPIC_TEMP_AUTO)) {
    int16_t temp;

    temp = strtod((char *) payload, NULL);
    if ((temp >= 18) || (temp <= 35)) {
      tempMin = temp;
      tempMax = temp + THERMOSTAT_HYSTERSIS;
#ifdef DEBUG
      Serial.print("Thermostat new temperature: ");
      Serial.println(tempMin);
#endif
    }
    return true;
  }
  return false;
} // boolean mqtt_thermostat(char *topicCut, char *payload)
