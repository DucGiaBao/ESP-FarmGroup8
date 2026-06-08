#include <Arduino.h>
#include <DHT.h>
#include "adafruit_io_client.h"
#include "database_client.h"
#include "io_functions.h"

#define SOIL_PIN 34
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Adjust after calibration
const int dryValue = 3200;
const int wetValue = 1400;

const IOConfig ioConfig = {
    2, // Alert LED pin
    21, // I2C SDA pin
    22, // I2C SCL pin
    33.0f, // Temperature alert on 
    32.0f  // Temperature alert off to avoid flickering
};

// Upload to Adafruit IO every 10 seconds
const unsigned long uploadIntervalMs = 10000;
unsigned long lastUploadMs = 0;  

void setup() {
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(12); 
    dht.begin();

    beginIOFunctions(ioConfig);
    showIOSplashScreen();

    Serial.println("ESP32 + DHT11 + Soil Moisture Sensor");
    Serial.print("Temperature alert LED pin: ");
    Serial.println(ioConfig.alertLedPin);
    Serial.print("Alert turns ON at ");
    Serial.print(ioConfig.tempAlertOnC);
    Serial.println(" C");

    beginAdafruitIO();
    beginDatabaseClient();
}

void loop() {
    updateAdafruitIO();

    bool dhtEnabled = isDhtSensorEnabled();
    bool soilEnabled = isSoilSensorEnabled();

    float humidity = 0.0f;
    float temperatureC = 0.0f;
    bool dhtOk = false;
    if (dhtEnabled) {
        humidity = dht.readHumidity();
        temperatureC = dht.readTemperature();
        dhtOk = !(isnan(humidity) || isnan(temperatureC));
    }

    int soilRaw = 0;
    int soilPercent = 0;
    if (soilEnabled) {
        soilRaw = analogRead(SOIL_PIN);
        soilPercent = map(soilRaw, dryValue, wetValue, 0, 100);
        soilPercent = constrain(soilPercent, 0, 100);
    }

    updateTempAlertLED(temperatureC, dhtEnabled && dhtOk);

    if (dhtEnabled && !dhtOk) {
        Serial.println("Failed to read from DHT11");
    } else if (dhtEnabled) {
        Serial.print("Temperature: ");
        Serial.print(temperatureC);
        Serial.println(" C");

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");
    } else {
        Serial.println("DHT11 sensor disabled by remote control.");
    }

    if (soilEnabled) {
        Serial.print("Soil Raw: ");
        Serial.print(soilRaw);
        Serial.print(" | Soil Moisture: ");
        Serial.print(soilPercent);
        Serial.println(" %");
    } else {
        Serial.println("Soil sensor disabled by remote control.");
    }

    Serial.println("------------------------");

    printLCD(temperatureC,
             humidity,
             soilRaw,
             soilPercent,
             dhtEnabled,
             dhtOk,
             soilEnabled);

    if (millis() - lastUploadMs >= uploadIntervalMs) {
        lastUploadMs = millis();
        sendToAdafruitIO(temperatureC,
                         humidity,
                         dhtEnabled,
                         dhtOk,
                         soilRaw,
                         soilPercent,
                         soilEnabled);

        sendToDatabase(temperatureC,
                       humidity,
                       dhtEnabled,
                       dhtOk,
                       soilRaw,
                       soilPercent,
                       soilEnabled,
                       millis());
    } else {
        // Still need to call io.run() regularly to maintain connection
        updateAdafruitIO();
    }

    delay(2000);
}