#include "arduino.h"
#include "mqttenv.h"
#include <ArduinoJson.h>         //https://github.com/bblanchon/ArduinoJson

Mqttenv::Mqttenv(MqttClient* mqtt) :
  _mqttclient(mqtt) {

}

void Mqttenv::begin() {
    Serial.print("Attempting to connect to the MQTT broker: ");
    Serial.println(_broker);
    while (!_mqttclient->connect(_broker, _port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(_mqttclient->connectError());
        delay (10000);
    }
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
}

void Mqttenv::reconnect() {
  if (!_mqttclient->connected()) {
    Serial.println("MQTT dissconnected");
      while (!_mqttclient->connect(_broker, _port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(_mqttclient->connectError());
        delay (10000);
      }
  }
}

void Mqttenv::send(float co2, float tvoc) {
  // Create a StaticJsonDocument with a capacity of 256 bytes
  StaticJsonDocument<256> doc;

  String topic_dev = "environment/";
  topic_dev += "ccs811/";
  topic_dev += "ccs811";
  
  doc["tvoc"] = tvoc;
  doc["eco2"] = co2;
  // Serialize the JSON document to a char buffer
  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  const char* payload = jsonBuffer;
  //mqttClient.publish(topic_dev, payload);
  _mqttclient->beginMessage(topic_dev);
  _mqttclient->print(payload);
  _mqttclient->endMessage();
  Serial.println("Published message: "+ topic_dev + String(payload));
}