# env_sensor
esp32 sensors project

Environmental sensor using a esp 32.

Sensors supported:
 - CCS811 MOX sensor, eCO2, eTVOC
 - HTU21DF Temperature and humidity sensor
 
Uses Autoconnect library for wifi configuration, soft ap to enable simple connection to different wifi.
Also supports OTA from the autoconnect library.

Sends data over MQTT.

Sends discovery messages to homeassistant to automatically configure sensors.
