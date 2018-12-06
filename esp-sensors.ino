// ESP-8266 as MQTT sensors platform

#include "esp-sensors.h"
#include "mqtt-support.h"
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "dimmer.h"
#include "moveir.h"
#include "internal.h"
#include "sonar.h"
#include "switch.h"
#include "thermostat.h"
#include "weather.h"

// Configure web server
ESP8266WebServer server(80);

// Wifi and MQTT connections
WiFiClient wifi;
PubSubClient myClient(wifi);

// Configure mode flag (create AP with web server to configure wifi and mqtt server)
uint8_t FlagConfigure;
boolean signatureConf = false;

// Full MQTT prefix (with chip serial ID)
char mqttPrefixNameS[32];
uint8_t mqttPrefixNameLength;

// MQTT Client name (with chip serial ID)
char mqttClientName[32];

// Wifi and MQTT connection parameters
char wifiSsid[32];
char wifiPass[16];
char mqttServer[16];

// Connected to MQTT flag
boolean connected = false;

// MQTT callback function (incoming messages)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char *topicCut;
  char payloadBuffer[32];
  int lengthBuffer;

  lengthBuffer = length;
  if (length >= sizeof(payloadBuffer))
    lengthBuffer = sizeof(payloadBuffer) - 1;
  memcpy(payloadBuffer, payload, lengthBuffer);
  payloadBuffer[lengthBuffer] = 0;
  topicCut = topic + mqttPrefixNameLength;
#ifdef DEBUG
  Serial.print("Topic received: ");
  Serial.println(topicCut);
  Serial.print("Payload: ");
  Serial.println((char *) payloadBuffer);
#endif

  // Now check all modules if the message for one of them
  if (mqtt_internal(topicCut, payloadBuffer))
    return;

#ifdef MOVEMENT_CONTROL
  if (mqtt_moveir(topicCut, payloadBuffer))
    return;
#endif
#ifdef DIMMER_CONTROL
  if (mqtt_dimmer(topicCut, payloadBuffer))
    return;
#endif
#if defined(MAXBOTIX_CONTROL) || defined(HCSR04_CONTROL)
  if (mqtt_sonar(topicCut, payloadBuffer))
    return;
#endif
#ifdef DHT22_CONTROL
  if (mqtt_weather(topicCut, payloadBuffer))
    return;
#endif
#ifdef SONOFF_CONTROL
  if (mqtt_switch(topicCut, payloadBuffer))
    return;
#endif
#ifdef THERMOSTAT_CONTROL
  if (mqtt_thermostat(topicCut, payloadBuffer))
    return;
#endif
#ifdef DEBUG
  Serial.println("Unknown message, no modules for it!");
#endif
} // void mqttCallback(char* topic, byte* payload, unsigned int length)

// Subscribe and publish Plug&Play information to MQTT
void pnpAndSubscribe() {
  // Reset & reconnect flags pnp information
  pnp_mqtt(TOPIC_REBOOT, "Reboot alarm flag", "Builtin Switches Alarm", "O:Switch", NULL, NULL);
  pnp_mqtt(TOPIC_RECONNECT, "Reconnect alarm flag", "Builtin Switches Alarm", "O:Switch", NULL, NULL);

  // All modules subscribe & PnP
  pnp_internal();
  subscribe_internal();
#ifdef MOVEMENT_CONTROL
  pnp_moveir();
  subscribe_moveir();
#endif
#ifdef DIMMER_CONTROL
  pnp_dimmer();
  subscribe_dimmer();
#endif
#if defined(MAXBOTIX_CONTROL) || defined(HCSR04_CONTROL)
  pnp_sonar();
  subscribe_sonar();
#endif
#ifdef DHT22_CONTROL
  pnp_weather();
  subscribe_weather();
#endif
#ifdef SONOFF_CONTROL
  pnp_switch();
  subscribe_switch();
#endif
#ifdef THERMOSTAT_CONTROL
  pnp_thermostat();
  subscribe_thermostat();
#endif
} // void pnpAndSubscribe()

// Use chip ID to generate unique MQTT prefix
void generateMqttName() {
  sprintf(mqttPrefixNameS, "%s%08X/", MQTT_PREFIX, ESP.getChipId());
  mqttPrefixName = mqttPrefixNameS;
  mqttPrefixNameLength = strlen(mqttPrefixNameS);
  sprintf(mqttClientName, "%s%08X", MQTT_NAME, ESP.getChipId());
#ifdef DEBUG
  Serial.print("Auto-generated MQTT prefix: ");
  Serial.println(mqttPrefixName);
  Serial.print("Auto-generated MQTT name: ");
  Serial.println(mqttClientName);
#endif
} // void generateMQTTName()

// When we started configure mode
unsigned long startConfigure = 0;
// When we started wifi fconnecting
unsigned long startConnecting = 0;
// Reboot and reconnects report timers (if == 1 then report it for 10 seconds)
unsigned long lastReboot = 1;
unsigned long lastReconnect = 1;

String content;
int statusCode;

// Create web server in configure mode
void createWebServer() {
  server.begin();
  server.on("/", []() {
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
    content += ipStr;
    content += "<p>";
    content += "</p><form method='get' action='setting'><label>SSID:&nbsp;</label><input name='ssid' length=32>";
    content += "<BR><label>Password:&nbsp;</label><input name='pass' length=16>";
    content += "<BR><label>MQTT&nbsp;broker&nbsp;IP:&nbsp;</label><input name='mqttip' length=15><P><input type='submit'></form>";
    content += "</html>";
    server.send(200, "text/html", content);
    startConfigure = millis();
  });
  server.on("/setting", []() {
    String qsid = server.arg("ssid");
    String qpass = server.arg("pass");
    String mqttip = server.arg("mqttip");
    if ((qsid.length() > 0) && (qpass.length() > 0) && (mqttip.length() > 0)) {
#ifdef DEBUG
      Serial.println("Writing to EEPROM");
      Serial.println("writing eeprom ssid:");
#endif
      EEPROM.begin(512);
      // Write signature
      EEPROM.write(0, 0xA5);
      strncpy(wifiSsid, qsid.c_str(), sizeof(wifiSsid) - 1);
      wifiSsid[sizeof(wifiSsid) - 1] = '\0';
      strncpy(wifiPass, qpass.c_str(), sizeof(wifiPass) - 1);
      wifiPass[sizeof(wifiPass) - 1] = '\0';
      strncpy(mqttServer, mqttip.c_str(), sizeof(mqttServer) - 1);
      mqttServer[sizeof(mqttServer) - 1] = '\0';
      for (int i = 0; i < sizeof(wifiSsid); i++) {
        EEPROM.write(i + 1, wifiSsid[i]);
#ifdef DEBUG
        Serial.print("Wrote: ");
        Serial.println(wifiSsid[i]);
#endif
      }
#ifdef DEBUG
      Serial.println("writing eeprom pass:");
#endif
      for (int i = 0; i < sizeof(wifiPass); i++) {
        EEPROM.write(i + sizeof(wifiSsid) + 1, wifiPass[i]);
#ifdef DEBUG
        Serial.print("Wrote: ");
        Serial.println(wifiPass[i]);
#endif
      }
#ifdef DEBUG
      Serial.println("writing eeprom mqtt broker ip:");
#endif
      for (int i = 0; i < sizeof(mqttServer); i++) {
        EEPROM.write(i + sizeof(wifiSsid) + sizeof(wifiPass) + 1, mqttServer[i]);
#ifdef DEBUG
        Serial.print("Wrote: ");
        Serial.println(mqttServer[i]);
#endif
      }
      EEPROM.commit();
      EEPROM.end();
      content = "{\"Success\":\"saved to eeprom... restarting to boot into new wifi\"}";
      statusCode = 200;
      server.send(statusCode, "application/json", content);
      FlagConfigure = 0;
      delay(1000);
      WiFi.disconnect();
      ESP.restart();
    } else {
      content = "{\"Error\":\"404 not found\"}";
      statusCode = 404;
      Serial.println("Sending 404");
      server.send(statusCode, "application/json", content);
    }
  });
} // void createWebServer()

// Connect to wifi
void setupWifi(boolean configureNetwork) {
  int numAttempts = 0;

  connected = false;
#ifdef DEBUG
  Serial.print("wifi status (before disconnect): ");
  Serial.println(WiFi.status());
#endif
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(10);

  if (!configureNetwork) {
    // We start by connecting to a WiFi network
#ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(wifiSsid);
#endif

    FlagConfigure = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid, wifiPass);
    startConnecting = millis();
  } else {
#ifdef DEBUG
    Serial.println("Switching to configure mode!");
#endif
    FlagConfigure = 1;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
#ifdef DEBUG
    Serial.print("Create AP to configure wifi parameters, SSID=");
    Serial.println(mqttClientName);
#endif
    WiFi.mode(WIFI_AP);
    WiFi.softAP(mqttClientName, WIFI_CONFIGURE_PASS);
    createWebServer();
    startConfigure = millis();
  }
} // void setup_wifi()

// Check MQTT and Wifi connection, reconnect if needed
void checkConnection() {
  int numAttempts = 0;

  if (WiFi.status() != WL_CONNECTED) {
    // Check if we lost connection just
    if (connected) {
      connected = false;
      setupWifi(false);
      return;
    }

    if ((millis() - startConnecting) > 60000) {
      if (!FlagConfigure)
        setupWifi(true);
      else {
        if (signatureConf && ((millis() - startConfigure) > 180000))
          setupWifi(false);
      }
    }
    return;
  }

  if (!connected) {
    // OTA setup
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(mqttClientName);
    ArduinoOTA.setPassword(OTA_UPLOAD_PASS);
    ArduinoOTA.begin();
  }

  // Loop until we're reconnected
  while (!check_mqtt()) {
#ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
#endif
    lastReconnect = 1;

    // Attempt to connect to MQTT broker
    if (connect_mqtt(mqttServer, mqttClientName, MQTT_USER_NAME, MQTT_PASSWORD, mqttCallback)) {
#ifdef DEBUG
      Serial.println("connected to MQTT");
      Serial.println("Resubscribing to MQTT topics");
#endif
      pnpAndSubscribe();
    } else {
#ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(myClient.state());
      Serial.println(" try again in 5 seconds");
#endif
      // Wait 5 seconds before retrying
#ifndef DISABLE_BLINK
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED, LOW);
        delay(500);
        digitalWrite(LED, HIGH);
        delay(500);
      }
#else
      delay(1000);
#endif
      numAttempts++;
      // Check if we have enough of this and start in configure mode
      if (numAttempts > 5) {
#ifdef DEBUG
        Serial.println("Switching to configure mode!");
#endif
        setupWifi(true);
        return;
      }
    }
  }
  connected = true;
} // void check_connection()

// Main setup function
void setup() {
  lastReboot = 1;
  Serial.begin(38400);
#ifdef DEBUG
  Serial.println("Reboot");
#endif
  generateMqttName();

  // Setup for all modules
  setup_internal();
#ifdef MOVEMENT_CONTROL
  setup_moveir();
#endif
#ifdef DIMMER_CONTROL
  setup_dimmer();
#endif
#if defined(MAXBOTIX_CONTROL) || defined(HCSR04_CONTROL)
  setup_sonar();
#endif
#ifdef DHT22_CONTROL
  setup_weather();
#endif
#ifdef SONOFF_CONTROL
  setup_switch();
#endif
#ifdef THERMOSTAT_CONTROL
  setup_thermostat();
#endif

  EEPROM.begin(512);
#ifdef DEBUG
  Serial.print("Signature in EEPROM: ");
  Serial.println(EEPROM.read(0));
#endif
  // Check if we have stored configuration data
  if (EEPROM.read(0) != 0xA5) {
    // No parameters in EEPROM, start configuration mode
    signatureConf = false;
    setupWifi(true);
  } else {
    signatureConf = true;
    for (int i = 0; i < sizeof(wifiSsid); i++)
      wifiSsid[i] = EEPROM.read(i + 1);
    for (int i = 0; i < sizeof(wifiPass); i++)
      wifiPass[i] = EEPROM.read(i + sizeof(wifiSsid) + 1);
    for (int i = 0; i < sizeof(mqttServer); i++)
      mqttServer[i] = EEPROM.read(i + sizeof(wifiSsid) + sizeof(wifiPass) + 1);
#ifdef DEBUG
    Serial.print("Found WIFI parameters in EEPROM: ");
    Serial.print(wifiSsid);
    Serial.print(" ");
    Serial.print(wifiPass);
    Serial.print(" ");
    Serial.println(mqttServer);
#endif
    setupWifi(false);
  }
  EEPROM.end();
} // void setup()

// Force send gratious ARP to prevent losing network
extern "C" {
  char *netif_list;
  uint8_t etharp_request(char *, char *);
}

unsigned long lastARP = 0;

void forceARP() {
  char *netif = netif_list;

  if ((millis() - lastARP) < 500)
    return;
  lastARP = millis();

  while (netif) {
    etharp_request((netif), (netif + 4));
    netif = *((char **) netif);
  }
} // void forceARP()


unsigned long lastRefresh = 0;
unsigned long lastLEDToggle = 0;
unsigned long lastForce = 0;

// Main loop function
void loop() {
  unsigned long lastCycle = millis();
  int delayNeeded;

  // Check Wifi and MQTT connections
  checkConnection();

  // Loop check for all modules (even if not connected)
  loop_internal();
#ifdef MOVEMENT_CONTROL
  loop_moveir();
#endif
#ifdef DIMMER_CONTROL
  loop_dimmer();
#endif
#if defined(MAXBOTIX_CONTROL) || defined(HCSR04_CONTROL)
  loop_sonar();
#endif
#ifdef DHT22_CONTROL
  loop_weather();
#endif
#ifdef SONOFF_CONTROL
  loop_switch();
#endif
#ifdef THERMOSTAT_CONTROL
  loop_thermostat();
#endif

  // MQTT loop check
  if (check_mqtt())
    mqtt_loop();

  // Arduino OTA check and force send gratious ARP
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    forceARP();
  }


  // Periodic call to refresh functions (if we're connected only!)
  if (check_mqtt() && ((millis() - lastRefresh) > REFRESH_TIME)) {
    boolean flagForce = false;
    
    lastRefresh = millis();

    if ((millis() - lastForce) > FORCE_TIME) {
      lastForce = millis();
      flagForce = true;
    }

    // Repeat reconnect and reboot flags for 10 seconds after it is on
    if (lastReboot) {
      if (lastReboot == 1)
        lastReboot = millis();

      if ((millis() - lastReboot) < 10000) {
        publish_mqttS(TOPIC_REBOOT, "ON");
      } else {
        // reset reboot report flag
        publish_mqttS(TOPIC_REBOOT, "OFF");
        lastReboot = 0;
      }
    } else
      if (flagForce)
        publish_mqttS(TOPIC_REBOOT, "OFF");
    if (lastReconnect) {
      if (lastReconnect == 1)
        lastReconnect = millis();

      if ((millis() - lastReconnect) < 10000) {
        publish_mqttS(TOPIC_RECONNECT, "ON");
      } else {
        // reset reconnect report flag
        publish_mqttS(TOPIC_RECONNECT, "OFF");
        lastReconnect = 0;
      }
    } else
      if (flagForce)
        publish_mqttS(TOPIC_RECONNECT, "OFF");
    

    // Refreshing all modules
    refresh_internal(flagForce);
#ifdef MOVEMENT_CONTROL
    refresh_moveir(flagForce);
#endif
#ifdef DIMMER_CONTROL
    refresh_dimmer(flagForce);
#endif
#if defined(MAXBOTIX_CONTROL) || defined(HCSR04_CONTROL)
    refresh_sonar(flagForce);
#endif
#ifdef DHT22_CONTROL
    refresh_weather(flagForce);
#endif
#ifdef SONOFF_CONTROL
    refresh_switch(flagForce);
#endif
#ifdef THERMOSTAT_CONTROL
    refresh_thermostat(flagForce);
#endif
  }

#ifndef DISABLE_BLINK
  // Blink LED at connect/configure
  if (!connected) {
    int delayBlink = 500;

    if (FlagConfigure)
      delayBlink = 75;
    if ((millis() - lastLEDToggle) > delayBlink) {
      lastLEDToggle = millis();
      ledState = !ledState;
    }
  }
#endif

  // Delay for 1..10 milliseconds (ESP-8266 internal routines need that)
  delayNeeded = 10 - (millis() - lastCycle);
  if (delayNeeded <= 0)
    delayNeeded = 1;
  delay(delayNeeded);
} // void loop()
