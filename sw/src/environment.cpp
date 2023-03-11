#include "Arduino.h"
#include "environment.h"

#include "SparkFunCCS811.h"
#include "ClosedCube_HDC1080.h"

Environment::Environment(int ccs811_addr, int hdc1080_addr) {
    _ccs811 = CCS811(ccs811_addr);
    _hdc1080_addr = hdc1080_addr;
};

void Environment::begin() {

    Wire.begin(); //Compilation will fail here if your hardware doesn't support additional Wire ports

    if (_ccs811.begin() == false)
    {
    Serial.print("CCS811 error. Please check wiring. Freezing...");
    while (1)
        ;
    }

    // Default settings: 
    //  - Heater off
	//  - 14 bit Temperature and Humidity Measurement Resolutions
    _hdc1080.begin(_hdc1080_addr);
}

void Environment::printInfo() {
    
	Serial.print("Manufacturer ID=0x");
	Serial.println(_hdc1080.readManufacturerId(), HEX); // 0x5449 ID of Texas Instruments
	Serial.print("Device ID=0x");
	Serial.println(_hdc1080.readDeviceId(), HEX); // 0x1050 ID of the device
	
    Serial.print("Device Serial Number=");
	HDC1080_SerialNumber sernum = _hdc1080.readSerialNumber();
	char format[15];
	sprintf(format, "%02X-%04X-%04X", sernum.serialFirst, sernum.serialMid, sernum.serialLast);
	Serial.println(format);
}

void Environment::getData() {
    _temp = _hdc1080.readTemperature();
    _humid = _hdc1080.readHumidity();
    
    //Check to see if data is ready with .dataAvailable()
    if (_ccs811.dataAvailable())
    {
        //If so, have the sensor read and calculate the results.
        //Get them later
        _ccs811.setEnvironmentalData(_humid, _temp);
        _ccs811.readAlgorithmResults();
        _co2 = _ccs811.getCO2();
        _tvoc = _ccs811.getTVOC();
    }
}

float Environment::getTemperature() {
    return _temp;
}

float Environment::getHumidity() {
    return _humid;
}

float Environment::getCo2() {
    return _co2;
}

float Environment::getTvoc() {
    return _tvoc;
}

void Environment::printData() {
    Serial.print("\nT=");
	Serial.print(_temp);
	Serial.print("C, RH=");
	Serial.print(_humid);
	Serial.print("% ");
    Serial.print("CO2=");
    //Returns calculated CO2 reading
    Serial.print(_co2);
    Serial.print(" tVOC=");
    //Returns calculated TVOC reading
    Serial.print(_tvoc);
    Serial.print(" millis=");
    //Display the time since program start
    Serial.print(millis());
    Serial.println(); 
}