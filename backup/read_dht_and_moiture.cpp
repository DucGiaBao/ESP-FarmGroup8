#include <Arduino.h>
#include <DHT.h>

#define SOIL_PIN 34
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Adjust after calibration
const int dryValue = 3200;
const int wetValue = 1400;

void setup() {
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(12); // 0-4095 on ESP32
    dht.begin();

    Serial.println("ESP32 + DHT11 + Soil Moisture Sensor");
}

void loop() {
    float humidity = dht.readHumidity();
    float temperatureC = dht.readTemperature();

    int soilRaw = analogRead(SOIL_PIN);
    int soilPercent = map(soilRaw, dryValue, wetValue, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    if (isnan(humidity) || isnan(temperatureC)) {
        Serial.println("Failed to read from DHT11");
    } else {
        Serial.print("Temperature: ");
        Serial.print(temperatureC);
        Serial.println(" °C");

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");
    }

    Serial.print("Soil Raw: ");
    Serial.print(soilRaw);
    Serial.print(" | Soil Moisture: ");
    Serial.print(soilPercent);
    Serial.println(" %");

    Serial.println("------------------------");
    delay(2000);
}