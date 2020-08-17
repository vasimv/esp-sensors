// Sonars support

#include "sonar.h"
#include "mqtt-support.h"
#ifdef MAXBOTIX_CONTROL
#include <SoftwareSerial.h>

SoftwareSerial mySerial =  SoftwareSerial(SW_rxPin, SW_txPin, false);
#endif

// publish PNP information to MQTT
void pnp_sonar() {
  pnp_mqtt(TOPIC_DISTANCE, "Distance", "Analogue Distance Sonar", "O:Number", "0", "500");
} // void pnp_sonar()

// subscribe to MQTT topics
void subscribe_sonar() {
} // void subscribe_sonar()

// Setup hardware (pins, etc)
void setup_sonar() {
#ifdef HCSR04_CONTROL
  pinMode(HCSR04_TRIG, OUTPUT);
  digitalWrite(HCSR04_TRIG, LOW);
  pinMode(HCSR04_ECHO, INPUT);
#else
#ifdef MAXBOTIX_CONTROL
  mySerial.begin(9600);
#endif
#endif
} // void setup_sonar()

#ifdef HCSR04_CONTROL
#define MIN_DISTANCE 3
int recursion = 0;

// Find distance with HC-SR04
int findDistanceSR() {
  uint32_t duration;
  int cm;

  digitalWrite(HCSR04_TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(HCSR04_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(HCSR04_TRIG, LOW);

  duration = pulseIn(HCSR04_ECHO, HIGH, 30000);
  cm = (duration / 2) / 29.1;
#ifdef DEBUG
  Serial.print("Distance = ");
  Serial.println(cm);
#endif
  if ((cm <= 0) || (cm > 600)) {
    pinMode(HCSR04_ECHO, OUTPUT);
    digitalWrite(HCSR04_ECHO, LOW);
    delay(100);
    pinMode(HCSR04_ECHO, INPUT);
    cm = findDistance();
    recursion++;
    if (recursion < 3)
      return findDistance();
  }
  recursion = 0;
  return cm;
} // int findDistanceSR()
#endif

#ifdef MAXBOTIX_CONTROL
#define MIN_DISTANCE 14
// Find distance with Maxbotix sonars
int findDistanceMB() {
  char rxBuf[16];
  char rcv;
  char *p;
  int cm = -1;

  p = rxBuf;
  while (mySerial.available()) {
    rcv = mySerial.read();
    switch (rcv) {
      case 'R':
        p = rxBuf;
        break;
      case '\n':
      case '\r':
        *p = '\0';
        cm = ((float) strtol(rxBuf, NULL, 10) * 2.54);
        p = rxBuf;
        break;
      default:
        *p = rcv;
        p++;
        if (p > (rxBuf + sizeof(rxBuf) - 1))
          p = rxBuf;
        break;
    }
    if (cm > 0)
      return cm;
  }
  return 0;
} // int findDistanceMB()
#endif

int findDistance() {
#ifdef HCSR04_CONTROL
  return findDistanceSR();
#endif
#ifdef MAXBOTIX_CONTROL
  return findDistanceMB();
#endif
  return 0;
} // int findDistance()

unsigned long lastSonar = 0;
int lastSonarState = -1;

// must be called from loop()
void loop_sonar() {
  int tmp;

  if ((millis() - lastSonar) > 200) {
    lastSonar = millis();
    tmp = findDistance();
    if ((tmp > 0) && (lastSonarState != tmp)) {
#ifdef DEBUG
      Serial.println("Reporting changed sonar distance");
#endif
      lastSonarState = tmp;
      publish_mqttI(TOPIC_DISTANCE, lastSonarState);
    }
  }
} // void loop_sonar()

// periodic refreshing values to MQTT
void refresh_sonar(boolean flagForce) {
  if (flagForce)
    publish_mqttI(TOPIC_DISTANCE, lastSonarState);
} // void refresh_sonar(boolean flagForce)

// hook for incoming MQTT messages
boolean mqtt_sonar(char *topicCut, char *payload) {
  return false;
} // boolean mqtt_sonar(char *topicCut, char *payload)
