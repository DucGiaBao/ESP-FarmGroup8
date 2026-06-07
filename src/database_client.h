#pragma once

#include <Arduino.h>

void beginDatabaseClient();
bool databaseClientConfigured();

bool sendToDatabase(float temperatureC,
                    float humidity,
                    bool dhtEnabled,
                    bool dhtOk,
                    int soilRaw,
                    int soilPercent,
                    bool soilEnabled,
                    unsigned long uptimeMs);
