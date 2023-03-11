#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>         //https://github.com/bblanchon/ArduinoJson

#include "environment.h"
#include "mqttenv.h"


  // Hardware Connections (Breakoutboard to Arduino):
  // 3.3V to 3.3V pin
  // GND to GND pin
  // SDA to A4
  // SCL to A5

//#define CCS811_ADDR 0x5B //Default I2C Address
#define CCS811_ADDR 0x5A //Alternate I2C Address
#define HDC1080_ADDR 0x40

Environment sensor(CCS811_ADDR, HDC1080_ADDR);

#include <U8g2lib.h>
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// MQTT stuff
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
Mqttenv mqttEnv(&mqttClient);

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;


void setup() {
  Serial.begin(115200);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  // wifiManager.autoConnect("AutoConnectAP");
  // or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  Serial.println("\n\nConnected.");
  Serial.println();
  
  server.begin();

  Serial.println("Initializing environment sensors.");

  Wire.begin(); //Compilation will fail here if your hardware doesn't support additional Wire ports
  sensor.begin();
  sensor.printInfo();

  Serial.println("Initializing display.");
  u8g2.begin();  

  mqttEnv.begin();
}

void showData() {
  char co2_string[20];
  char tvoc_string[20];
  char temp_string[20];

  sprintf(co2_string, "co2: %.1f", sensor.getCo2());
  sprintf(tvoc_string, "tvoc: %.1f", sensor.getTvoc());
  sprintf(temp_string, "T: %.1f H: %.1f", sensor.getTemperature(), sensor.getHumidity());

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0,20, co2_string);
    u8g2.drawStr(0,40, tvoc_string);
    u8g2.drawStr(0,60, temp_string);
  } while ( u8g2.nextPage() );
}

void sendData() {
  static uint32_t tick = 0;
  if ( millis() - tick < (60*1000)) { return; }
  // Do stuff here every 60 second
  tick = millis();

  sensor.printData();
  mqttEnv.reconnect();
  mqttEnv.send(sensor.getCo2(), sensor.getTvoc());
}

void loop() {
  sensor.getData();
  showData();
  sendData();
  delay(500);
}

