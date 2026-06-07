#pragma once
#include <Arduino.h>

void beginAdafruitIO();
void updateAdafruitIO();
bool adafruitIOConnected();
bool isDhtSensorEnabled();
bool isSoilSensorEnabled();

bool sendToAdafruitIO(float temperatureC,
                      float humidity,
                      bool dhtEnabled,
                      bool dhtOk,
                      int soilRaw,
                      int soilPercent,
                      bool soilEnabled);