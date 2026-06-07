#pragma once
#include <Arduino.h>

struct IOConfig {
    uint8_t alertLedPin;
    uint8_t i2cSdaPin;
    uint8_t i2cSclPin;
    float tempAlertOnC;
    float tempAlertOffC;
};

void beginIOFunctions(const IOConfig &config);
void showIOSplashScreen(); 
void updateTempAlertLED(float temperatureC, bool dhtOk);
void printLCD(float temperatureC,
              float humidity,
              int soilRaw,
              int soilPercent,
              bool dhtEnabled,
              bool dhtOk,
              bool soilEnabled);
