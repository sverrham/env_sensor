#ifndef Mqttenv_h
#define Mqttenv_h
#include "arduino.h"
#include <ESP8266WiFi.h>
#include <ArduinoMqttClient.h>

class Mqttenv {
public:
    Mqttenv(MqttClient*);
    void begin();
    void reconnect();
    void send(float co2, float tvoc);
private:
    MqttClient* _mqttclient;
    const char _broker[4] = "adg";
    int        _port     = 1883;
};

#endif