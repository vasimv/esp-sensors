// DHT22 weather sensor support

#include "weather.h"
#include "mqtt-support.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


// Last temperature read
float temperatureCurrent = 25;

// Last humidity read
float humidityCurrent = 75;

// publish PNP information to MQTT
void pnp_weather() {
  pnp_mqtt(TOPIC_DHT, "DHT22 enable", "DHT22 Weather Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_TEMPERATURE, "Temperature", "Analogue Weather DHT22 Temperature", "O:Number", "-40.0", "80.0");
  pnp_mqtt(TOPIC_HUMIDITY, "Humidity", "Analogue Weather DHT22 Humidity", "O:Number", "0.0", "100.0");
} // void pnp_weather()

// subscribe to MQTT topics
void subscribe_weather() {
  subscribe_mqtt(TOPIC_DHT);
} // void subscribe_weather()

DHT_Unified dht(DHTPIN, DHT22);
boolean FlagReportDHT = false;
uint32_t lastDHT = 0;

// Setup hardware (pins, etc)
void setup_weather() {
} // void setup_weather()

// Read temperature
void check_dht_temperature() {
  sensors_event_t event;
  char *p;
  int numDHTAttempts;

  // Crude fix to DHT22's problem with reading (just repeat few times)
  numDHTAttempts = 0;
  do {
    dht.temperature().getEvent(&event);
    numDHTAttempts++;
    if (isnan(event.temperature))
      delay(10);
  } while (isnan(event.temperature) && (numDHTAttempts < 5));
  
  if (!isnan(event.temperature)) {
#ifdef DEBUG
    Serial.print("Temperature: ");
    Serial.print(event.temperature);
    Serial.println("C");
#endif
    temperatureCurrent = event.temperature;
  }
} // void check_dht_temperature()

// Read humidity
void check_dht_humidity() {
  sensors_event_t event;
  char *p;

  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
#ifdef DEBUG
    Serial.print("Humidity: ");
    Serial.print(event.relative_humidity);
    Serial.println("%");
#endif
    humidityCurrent = event.relative_humidity;
  }
} // void check_dht_humidity()


// must be called from loop()
void loop_weather() {
  if (FlagReportDHT && ((millis() - lastDHT) > 10000)) {
    lastDHT = millis();
    check_dht_temperature();
    check_dht_humidity();
  }
} // void loop_weather()

// last values reported (to prevent spamming)
float lastTemperature = -1;
float lastHumidity = -1;

// periodic refreshing values to MQTT
void refresh_weather(boolean flagForce) {
  if (FlagReportDHT) {
    if (flagForce || (lastTemperature != temperatureCurrent)) {
      lastTemperature = temperatureCurrent;
      publish_mqttF(TOPIC_TEMPERATURE, temperatureCurrent);
    }
    if (flagForce || (lastHumidity != humidityCurrent)) {
      lastHumidity = humidityCurrent;
      publish_mqttF(TOPIC_HUMIDITY, humidityCurrent);
    }
  }
} // void refresh_weather(boolean flagForce)

// hook for incoming MQTT messages
boolean mqtt_weather(char *topicCut, char *payload) {
  if (cmpTopic(topicCut, TOPIC_DHT)) {
    if (cmpPayloadON(payload))
      FlagReportDHT = true;
    else
      FlagReportDHT = false;
#ifdef DEBUG
    Serial.print("DHT flag update: ");
    Serial.println(FlagReportDHT);
#endif
    return true;
  }
  return false;
} // boolean mqtt_weather(char *topicCut, char *payload)
