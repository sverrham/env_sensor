#ifndef Environment_h
#define Environment_h
#include "arduino.h"
#include "SparkFunCCS811.h"
#include "ClosedCube_HDC1080.h"

class Environment {
public:
    Environment(int ccs811_addr, int hdc1080_addr);
    void begin();
    void printInfo();
    void printData();
    void getData();
    float getTemperature();
    float getHumidity();
    float getCo2();
    float getTvoc();
private:
    float _temp;
    float _humid;
    float _co2;
    float _tvoc;
    int _hdc1080_addr;
    CCS811 _ccs811;
    ClosedCube_HDC1080 _hdc1080;
};

#endif