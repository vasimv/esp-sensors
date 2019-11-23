// DHT22 weather sensor support

#include "weather.h"
#include "mqtt-support.h"

#ifdef DHT22_CONTROL
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#endif

#ifdef GY39_CONTROL
#include <SoftwareSerial.h>
#endif


// Last temperature read
float temperatureCurrent = 25;

// Last humidity read
float humidityCurrent = 75;

// Last pressure read
float pressureCurrent = 100000;

// Last illumination read
float illuminationCurrent = 100;

// last values reported (to prevent spamming)
float lastTemperature = -1;
float lastHumidity = -1;
float lastPressure = -1;
float lastIllumination = -1;

// publish PNP information to MQTT
void pnp_weather() {
#ifdef DHT22_CONTROL
  pnp_mqtt(TOPIC_DHT, "DHT22 enable", "DHT22 Weather Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_TEMPERATURE, "Temperature", "Analogue Weather DHT22 Temperature", "O:Number", "-40.0", "80.0");
  pnp_mqtt(TOPIC_HUMIDITY, "Humidity", "Analogue Weather DHT22 Humidity", "O:Number", "0.0", "100.0");
#endif
#ifdef GY39_CONTROL
  pnp_mqtt(TOPIC_GY39, "GY-39 enable", "GY39 Weather Switches", "I:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_TEMPERATURE, "Temperature", "Analogue Weather GY39 Temperature", "O:Number", "-40.0", "80.0");
  pnp_mqtt(TOPIC_HUMIDITY, "Humidity", "Analogue Weather GY39 Humidity", "O:Number", "0.0", "100.0");
  pnp_mqtt(TOPIC_ILLUMINATION, "Illumination", "Analogue Weather GY39 Illumination Light", "O:Number", "0.0", "190000.0");
  pnp_mqtt(TOPIC_PRESSURE, "Pressure", "Analogue Weather GY39 Pressure", "O:Number", "0.0", "200000.0");
#endif
} // void pnp_weather()

// subscribe to MQTT topics
void subscribe_weather() {
#ifdef DHT22_CONTROL
  subscribe_mqtt(TOPIC_DHT);
#endif
#ifdef GY39_CONTROL
  subscribe_mqtt(TOPIC_GY39);
#endif
} // void subscribe_weather()

boolean FlagReportWeather = false;

// hook for incoming MQTT messages
boolean mqtt_weather(char *topicCut, char *payload) {
  if (cmpTopic(topicCut, TOPIC_DHT) || cmpTopic(topicCut, TOPIC_GY39)) {
    if (cmpPayloadON(payload))
      FlagReportWeather = true;
    else
      FlagReportWeather = false;
#ifdef DEBUG
    Serial.print("Weather flag update: ");
    Serial.println(FlagReportWeather);
#endif
    return true;
  }
  return false;
} // boolean mqtt_weather(char *topicCut, char *payload)

#ifdef DHT22_CONTROL
DHT_Unified dht(DHTPIN, DHT22);
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
  if (FlagReportWeather && ((millis() - lastDHT) > 10000)) {
    lastDHT = millis();
    check_dht_temperature();
    check_dht_humidity();
  }
} // void loop_weather()

// periodic refreshing values to MQTT
void refresh_weather(boolean flagForce) {
  if (FlagReportWeather) {
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
#endif

#ifdef GY39_CONTROL
SoftwareSerial sUart(GY39PIN, SW_SERIAL_UNUSED_PIN, false, 64,0);

// Setup hardware (pins, etc)
void setup_weather() {
  sUart.begin(9600);
} // void setup_weather()

// Output variables
// Light level data ready
boolean gy39ReadyLight = false;
// Light level data (in lux, lumen per square meter)
float gy39Lux;
// Environment data ready
boolean gy39ReadyEnv = false;
// Temperature (in Celsius)
float gy39Temperature;
// Humidity (in percents)
float gy39Humidity;
// Pressure (in Pa)
float gy39Pressure;
// Altitude above sea level (in meters), very estimate
float gy39Altitude;

// Internal variables of processGY39
int gy39State = 0; 
int gy39Type;
int countCharGY39 = 0;
unsigned int gy39Checksum = 0;
int countWaitGY39 = 0;
unsigned long lastGY39Char = 0;
uint8_t bufGY39[11];


// process character from GY-39
void processGY39(char c) {
  // Reset if we wait too long (more than second)
  if ((millis() - lastGY39Char) > 1000) {
    lastGY39Char = millis();
    gy39State = 0;
    countCharGY39 = 0;
  }
  
  // Serial.printf("gy39 PRE state: %d, countchar: %d\n", gy39State, countCharGY39);
  switch (gy39State) {
    // Waiting for ZZ (begin of packet)
    case 0:
      // Not 'Z', reset counter
      if (c != 'Z') {
          countCharGY39 = 0;
          break;
      }
      if (countCharGY39 == 1) {
        gy39State = 1;
        countCharGY39 = 0;
      } else {
        // Reset checksum, wait for second 'Z'
        gy39Checksum = 0;
        countCharGY39 = 1;
      }
      break;
    // Get packet type
    case 1:
      // Unknown packet type, reset
      if ((c != 0x15) && (c != 0x45)) {
        gy39State = 0;
        countCharGY39 = 0;
        break;
      }
      gy39Type = c;
      gy39State = 2;
      break;
    // Get number of bytes
    case 2:
      // Wrong length for packet, reset
      if (((gy39Type == 0x15) && (c != 0x04)) || ((gy39Type == 0x45) && (c != 0x0A))) {
        gy39State = 0;
        countCharGY39 = 0;
        break;
      }
      gy39State = 3;
      countWaitGY39 = (int) c + 1;
      countCharGY39 = 0;
      break;
    // Receive data part of packet with checksum
    case 3:
      // Put char in buffer
      bufGY39[countCharGY39] = (uint8_t) c;
      countCharGY39++;
      // Check if have received full packet with checksum
      if (countCharGY39 >= countWaitGY39) {
        // Prepare for next packet
        gy39State = 0;
        countCharGY39 = 0;
        // Wrong checksum, do nothing
        if (bufGY39[countWaitGY39 - 1] != (gy39Checksum & 0xff))
          break;
        // Parse data
        if (gy39Type == 0x15) {
          // 0x15 - Light
          gy39Lux = ((float) ((((uint32_t) bufGY39[0]) << 24) | (((uint32_t) bufGY39[1]) << 16) | (((uint32_t) bufGY39[2]) << 8) | (uint32_t) bufGY39[0])) / 100.0f;
          gy39ReadyLight = true; 
        } else {
          // 0x45 - Environment temperature/pressure/humidity
          gy39Temperature = ((float) ((((uint16_t) bufGY39[0]) << 8) | (uint16_t) bufGY39[1])) / 100.0f;
          gy39Pressure = ((float) ((((uint32_t) bufGY39[2]) << 24) | (((uint32_t) bufGY39[3]) << 16) | (((uint32_t) bufGY39[4]) << 8) | (uint32_t) bufGY39[5])) / 100.0f;
          gy39Humidity = ((float) ((((uint16_t) bufGY39[6]) << 8) | (uint16_t) bufGY39[7])) / 100.0f;
          gy39Altitude = ((float) ((((uint16_t) bufGY39[8]) << 8) | (uint16_t) bufGY39[9])) / 100.0f;
          gy39ReadyEnv = true;
        }
      }
      break;
    default:
      // Shouldn't happen
      gy39State = 0;
      countCharGY39 = 0;
      break;
  }
  gy39Checksum += (unsigned int) c;
  return;
} // void processGY39(char c)

// must be called from loop()
void loop_weather() {
  if (FlagReportWeather)
    while (sUart.available())
      processGY39(sUart.read());
} // void loop_weather()

// periodic refreshing values to MQTT
void refresh_weather(boolean flagForce) {
  if (FlagReportWeather) {
    if (gy39ReadyLight) {
      gy39ReadyLight = false;
      illuminationCurrent = gy39Lux;
    }
    if (gy39ReadyEnv) {
      gy39ReadyEnv = false;
      temperatureCurrent = gy39Temperature;
      humidityCurrent = gy39Humidity;
      pressureCurrent = gy39Pressure;
    }
    if (flagForce || (lastTemperature != temperatureCurrent)) {
      lastTemperature = temperatureCurrent;
      publish_mqttF(TOPIC_TEMPERATURE, temperatureCurrent);
    }
    if (flagForce || (lastHumidity != humidityCurrent)) {
      lastHumidity = humidityCurrent;
      publish_mqttF(TOPIC_HUMIDITY, humidityCurrent);
    }
    if (flagForce || (lastPressure != pressureCurrent)) {
      lastPressure = pressureCurrent;
      publish_mqttF(TOPIC_PRESSURE, pressureCurrent);
    }
    if (flagForce || (lastIllumination != illuminationCurrent)) {
      lastIllumination = illuminationCurrent;
      publish_mqttF(TOPIC_ILLUMINATION, illuminationCurrent);
    }
  }
} // void refresh_weather(boolean flagForce)
#endif
