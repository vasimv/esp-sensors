This arduino firmware turns ESP-8266 into MQTT-enabled plug&play sensor platform. It allows to receive updates and control for following sensors and devices:

* Movement detection sensors (optical IR sensors and Parralax X-Band motion detector, up to 3 at one device)
DHT22 weather sensors
* 4-ch LED strips dimmer through Modbus/RS-485: http://github.com/vasimv/StmDimmer-4ch
* Internal button and LED 
* Simple thermostat (requires DHT-22) with two relays to control indoor (fan) and outdoor (compressor) units
* HC-SR04 and Maxbotix serial sonars
* Sonoff basic switch and Sonoff touch wall switch (last one can be configured to automatically turn on relay when pressed button or to control it through OpenHAB or HomeAssistant)

Plug&Play feature uses scripts (see PnP-scripts directory) to update OpenHAB or HomeAssistant configuration when new device with the firmware is added to network by calling the script (manually or by cron). For openHAB it'll create items and groups file to include all sensors connected to the device.

Other features are On-The-Air firmware update through ArduinoOTA, automatically switching to configuration mode to set WiFi-network authentication and MQTT broker's IP address, reporting to MQTT when the device is rebooted or reconnected to the MQTT broker.


CONFIGURING

Before compile you have to update file esp-sensors.h to uncomment modules you need and comment other ones (some modules are conflicting with eath other, see comments). The firmware needs some arduino libraries installed - Adafruit unified DHT22, PubSubClient.

At first start - it'll switch to configuration mode to set Wifi and MQTT parameters. The device will start its own WiFi network with name like "espx-xxxxxxxx". You must connect to the network (default password is "passtest"), then go to "http://192.168.4.1" and set WiFi network name, password and MQTT broker's IP address (default username/password for MQTT is "testuser"/"testtest", you can redefine them in esp-sensors.h at compilation).

If the device loses the network for long time (more than 60 seconds) - it'll turn configuration mode for 3 minutes (after this it'll try to connect to your network again).


ADVANCED

It isn't hard to add other sensors/devices to this firmware, you have to create module with few functions to control it and report its statuses through MQTT. Modules are defined in source files:

Sonars - sonar.h/sonar.cpp
Movement detection - moveir.h/moveir.cpp
Internal LED and button - internal.h/internal.cpp
Sonoff basic and touch - switch.h/switch.cpp
Thermostat - thermostat.h/thermostat.cpp
Dimmer - dimmer.h/dimmer.cpp
DHT22 - weather.h/weather.cpp

