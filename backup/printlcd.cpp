#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define SOIL_PIN 34
#define DHTPIN 4
#define DHTTYPE DHT11

// I2C pins for generic ESP32
#define I2C_SDA 21
#define I2C_SCL 22

DHT dht(DHTPIN, DHTTYPE);

// LCD 2004A = 20 columns, 4 rows
// Common I2C addresses are often 0x27 or 0x3F
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Adjust after calibration
const int dryValue = 3200;
const int wetValue = 1400;

void printLCD(float temperatureC, float humidity, int soilRaw, int soilPercent, bool dhtOk) {
    char line[21]; // 20 chars + null terminator

    // Row 0
    snprintf(line, sizeof(line), "ESP32 FARM MONITOR  ");
    lcd.setCursor(0, 0);
    lcd.print(line);

    // Row 1
    lcd.setCursor(0, 1);
    if (dhtOk) {
        snprintf(line, sizeof(line), "T:%4.1f%cC H:%4.0f%%   ",
                 temperatureC, 223, humidity);
    } else {
        snprintf(line, sizeof(line), "DHT11 read error    ");
    }
    lcd.print(line);

    // Row 2
    snprintf(line, sizeof(line), "Soil Raw: %-6d     ", soilRaw);
    lcd.setCursor(0, 2);
    lcd.print(line);

    // Row 3
    snprintf(line, sizeof(line), "Soil Moist: %3d%%   ", soilPercent);
    lcd.setCursor(0, 3);
    lcd.print(line);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(12); // 0-4095 on ESP32
    dht.begin();

    Wire.begin(I2C_SDA, I2C_SCL);

    lcd.init();
    lcd.backlight();
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("ESP32 + DHT11 +");
    lcd.setCursor(0, 1);
    lcd.print("Soil Sensor");
    delay(2000);
    lcd.clear();

    Serial.println("ESP32 + DHT11 + Soil Moisture Sensor");
}

void loop() {
    float humidity = dht.readHumidity();
    float temperatureC = dht.readTemperature();

    int soilRaw = analogRead(SOIL_PIN);
    int soilPercent = map(soilRaw, dryValue, wetValue, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    bool dhtOk = !(isnan(humidity) || isnan(temperatureC));

    if (!dhtOk) {
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

    printLCD(temperatureC, humidity, soilRaw, soilPercent, dhtOk);

    delay(2000);
}