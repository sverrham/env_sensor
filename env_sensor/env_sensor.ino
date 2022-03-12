#include <ESP8266WiFi.h>          // Replace with WiFi.h for ESP32
#include <ESP8266WebServer.h>     // Replace with WebServer.h for ESP32
#include <AutoConnect.h>
#include <Wire.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>

#include <SoftwareSerial.h>
#include <MHZ.h>

#include "Adafruit_CCS811.h"
#include "Adafruit_HTU21DF.h"

// Connect Vin to 3-5VDC
// Connect GND to ground
// Connect SCL to I2C clock pin (A5 on UNO, D1 on NodeMCU)
// Connect SDA to I2C data pin (A4 on UNO, D2 on NodeMCU)

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_CCS811 ccs;

// pin for pwm reading
//#define CO2_IN 13 //D7
// pin for uart reading
#define MH_Z19_RX 14  // D
#define MH_Z19_TX 12  // D

//MHZ mz19co2(MH_Z19_RX, MH_Z19_TX, CO2_IN, MHZ19B);
MHZ mz19co2(MH_Z19_RX, MH_Z19_TX, MHZ19B);


ESP8266WebServer Server;          // Replace with WebServer for ESP32
AutoConnect      Portal(Server);

AutoConnectConfig config;
AutoConnectAux    hello;

static const char HELLO_PAGE[] PROGMEM = R"(
{ "title": "About", "uri": "/", "menu": true, "element": [
    { "name": "caption", "type": "ACText", "value": "<h2>Env sensor</h2>",  "style": "text-align:center;color:#2f4f4f;padding:10px;" },
    { "name": "content", "type": "ACText", "value": "Temp sensor, Humidity sensor, eC02 sensor and eTVOC sensor." } ]
}
)";                                 // Step #5

unsigned int  updateInterval = 50000;
unsigned long lastPub = 0;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "192.168.86.180";
int        port     = 1883;
const char topic[]  = "arduino/simple";
char       unique_id[] = "00000000";

String sensorName = "TestSensor";
String stateTopic = "home/" + sensorName + "/state";

float humidity;
float temperature;
float co2;
float tvoc;
int co2_mhz;

void setup() {
  //pinMode(CO2_IN, INPUT);
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  
  config.ota = AC_OTA_BUILTIN;
  Portal.config(config);
  hello.load(HELLO_PAGE);
  Portal.join({ hello });
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }

  uint32_t chipid=ESP.getChipId();
  char clientid[25];
  snprintf(clientid,25,"WIFI-Display-%08X",chipid);
  snprintf(unique_id,8,"%08X",chipid);
  
  Serial.print("Client ID: ");
  Serial.println(clientid);
  
  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  mqttClient.setId(clientid);

  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword("mqttuser", "mqttuser");

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  } else {
    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
  }
  
  // Setup sensors
  Serial.println();
  Serial.println("HTU21D-F init");
  if (!htu.begin()) {
    Serial.println("Couldn't find sensor!");
  }

  Serial.println("CCS811 init");
  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
  }

  // Wait for the sensor to be ready
  while(!ccs.available());

  //mz19co2.setDebug(true);
  mz19co2.setAutoCalibrate(true);
}


void sendMQTTTemperatureDiscoveryMsg() {
  String unique_id_temp = String(unique_id) + "_temperature";
  String discoveryTopic = "homeassistant/sensor/" + unique_id_temp + "/temperature/config";
  
  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["name"] = sensorName + " Temperature";
  doc["stat_t"]   = stateTopic;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["frc_upd"] = true;
  doc["uniq_id"] = unique_id_temp;
  doc["val_tpl"] = "{{ value_json.temperature|default(0)|round(1) }}";

  size_t n = serializeJson(doc, buffer);
  
  mqttClient.beginMessage(discoveryTopic.c_str());
  mqttClient.print(buffer);
  mqttClient.endMessage();
  //mqttClient.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTHumidityDiscoveryMsg() {
  String unique_id_humid = String(unique_id) + "_humidity";
  String discoveryTopic = "homeassistant/sensor/" + unique_id_humid + "/humidity/config";

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["name"] = sensorName + " Humidity";
  doc["stat_t"]   = stateTopic;
  doc["unit_of_meas"] = "%";
  doc["dev_cla"] = "humidity";
  doc["frc_upd"] = true;
  doc["uniq_id"] = unique_id_humid;
  doc["val_tpl"] = "{{ value_json.humidity|default(0)|round(1) }}";

  size_t n = serializeJson(doc, buffer);

  mqttClient.beginMessage(discoveryTopic.c_str());
  mqttClient.print(buffer);
  mqttClient.endMessage();
  //mqttClient.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTCo2DiscoveryMsg() {
  String unique_id_c02 = String(unique_id) + "_co2";
  String discoveryTopic = "homeassistant/sensor/" + unique_id_c02 + "/co2/config";

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["name"] = sensorName + " CO2";
  doc["stat_t"]   = stateTopic;
  doc["unit_of_measurement"] = "ppm";
  doc["frc_upd"] = true;
  doc["ic"] = "mdi:molecule-co2";
  doc["uniq_id"] = unique_id_c02;
  doc["val_tpl"] = "{{ value_json.co2|default(0)|int }}";

  size_t n = serializeJson(doc, buffer);

  mqttClient.beginMessage(discoveryTopic.c_str());
  mqttClient.print(buffer);
  mqttClient.endMessage();
  //mqttClient.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTCo2MHZDiscoveryMsg() {
  String unique_id_c02 = String(unique_id) + "_co2mhz";
  String discoveryTopic = "homeassistant/sensor/" + unique_id_c02 + "/co2/config";

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["name"] = sensorName + " CO2 MHZ";
  doc["stat_t"]   = stateTopic;
  doc["unit_of_measurement"] = "ppm";
  doc["frc_upd"] = true;
  doc["ic"] = "mdi:molecule-co2";
  doc["uniq_id"] = unique_id_c02;
  doc["val_tpl"] = "{{ value_json.co2_mhz|default(0)|int }}";

  size_t n = serializeJson(doc, buffer);

  mqttClient.beginMessage(discoveryTopic.c_str());
  mqttClient.print(buffer);
  mqttClient.endMessage();
  //mqttClient.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTTvocDiscoveryMsg() {
  String unique_id_tvoc = String(unique_id) + "_tvoc";
  String discoveryTopic = "homeassistant/sensor/" + unique_id_tvoc + "/tvoc/config";

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["name"] = sensorName + " TVOC";
  doc["stat_t"]   = stateTopic;
  doc["unit_of_measurement"] = "ppb";
  doc["ic"] = "mdi:air-filter";
  doc["frc_upd"] = true;
  doc["uniq_id"] = unique_id_tvoc;
  doc["val_tpl"] = "{{ value_json.tvoc|default(0)|int }}";

  size_t n = serializeJson(doc, buffer);

  mqttClient.beginMessage(discoveryTopic.c_str());
  mqttClient.print(buffer);
  mqttClient.endMessage();
  //mqttClient.publish(discoveryTopic.c_str(), buffer, n);
}

void loop() {

  if (updateInterval > 0) {
    if (mqttClient.connected()) {
  
      if (millis() - lastPub > updateInterval) {

        if (mz19co2.isPreHeating()) {
          Serial.print("MZ19B Preheating ");
        }
  
        int ppm_uart = mz19co2.readCO2UART();
        if (ppm_uart > 0) {
          Serial.print("MZ19B: PPMuart: ");
          Serial.print(ppm_uart);
          co2_mhz = ppm_uart;
        } else {
          Serial.print("MZ19B: ");
          Serial.println(ppm_uart);
        }
      
//        int ppm_pwm = mz19co2.readCO2PWM();
//        Serial.print(", PPMpwm: ");
//        Serial.print(ppm_pwm);
      
        int temp_mz = mz19co2.getLastTemperature();
        if (temp_mz > 0) {
          Serial.print(", Temperature: ");
          Serial.println(temp_mz);
        } else {
          Serial.print("MZ19B: ");
          Serial.println(temp_mz);
        }

        sendMQTTTemperatureDiscoveryMsg();
        sendMQTTHumidityDiscoveryMsg();
        sendMQTTCo2DiscoveryMsg();
        sendMQTTCo2MHZDiscoveryMsg();
        sendMQTTTvocDiscoveryMsg();

        temperature = htu.readTemperature();
        humidity = htu.readHumidity();
        
        Serial.print("HTU21DF : ");
        Serial.print("T="); 
        Serial.print(temperature); 
        Serial.print("C,\tRH=");
        Serial.println(humidity);
      
        if(ccs.available()){
          if(!ccs.readData()){
            co2 = ccs.geteCO2();
            tvoc = ccs.getTVOC();
            Serial.print("CO2: ");
            Serial.print(co2);
            Serial.print("ppm,\tTVOC: ");
            Serial.println(tvoc);
          }
          else{
            Serial.println("ERROR!");
          }
        }

        DynamicJsonDocument doc(1024);
        char buffer[256];

        doc["humidity"] = humidity;
        doc["temperature"] = temperature;
        doc["co2"] = co2;
        doc["tvoc"] = tvoc;
        doc["co2_mhz"] = co2_mhz;
        
    
        size_t n = serializeJson(doc, buffer);
        
        mqttClient.beginMessage(stateTopic.c_str());
        mqttClient.print(buffer);
        mqttClient.endMessage();
        
  //      if (!mqttClient.connected()) {
  //        mqttConnect();
  //      }
  //      String item = String("field1=") + String(getStrength(7));
  //      mqttPublish(item);
  //      mqttClient.loop();
        lastPub = millis();
      }
    } else {
      Serial.println("reconnecting to mqttserver");
      mqttClient.connect(broker, port);
    }
  }

  Portal.handleClient();
}
