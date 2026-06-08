#include "adafruit_io_client.h"
#include "adafruit_secrets.h"
#include <AdafruitIO_WiFi.h>

// Create Adafruit IO connection
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
// Create/feed names in Adafruit IO
AdafruitIO_Feed *temperatureFeed = io.feed("temperature");
AdafruitIO_Feed *humidityFeed    = io.feed("humidity");
AdafruitIO_Feed *soilRawFeed     = io.feed("soil-raw");
AdafruitIO_Feed *soilMoistFeed   = io.feed("soil-moisture");
AdafruitIO_Feed *dhtEnableFeed   = io.feed("control-dht-enable");
AdafruitIO_Feed *soilEnableFeed  = io.feed("control-soil-enable");

bool dhtSensorEnabled = true;
bool soilSensorEnabled = true;

bool parseFeedToggle(AdafruitIO_Data *data) {
    String value = data->toString();
    value.trim();
    value.toLowerCase();

    if (value == "1" || value == "true" || value == "on" || value == "yes") {
        return true;
    }

    if (value == "0" || value == "false" || value == "off" || value == "no") {
        return false;
    }

    return data->toBool();
}

void handleDhtEnableMessage(AdafruitIO_Data *data) {
    dhtSensorEnabled = parseFeedToggle(data);
    Serial.print("Remote control - DHT11 sensor: ");
    Serial.println(dhtSensorEnabled ? "ENABLED" : "DISABLED");
}

void handleSoilEnableMessage(AdafruitIO_Data *data) {
    soilSensorEnabled = parseFeedToggle(data);
    Serial.print("Remote control - Soil sensor: ");
    Serial.println(soilSensorEnabled ? "ENABLED" : "DISABLED");
}

void beginAdafruitIO() {
    Serial.println("Connecting to Adafruit IO...");

    dhtEnableFeed->onMessage(handleDhtEnableMessage);
    soilEnableFeed->onMessage(handleSoilEnableMessage);

    io.connect();

    unsigned long start = millis();
    while (io.status() < AIO_CONNECTED && millis() - start < 30000) { // wait for 30 seconds max
        io.run();
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    if (io.status() == AIO_CONNECTED) {
        Serial.println("Adafruit IO connected!");
        Serial.println(io.statusText());

        // Pull the latest remote switch values after reconnect.
        dhtEnableFeed->get();
        soilEnableFeed->get();
    } else {
        Serial.println("Adafruit IO connection timeout.");
        Serial.println(io.statusText());
    }
}

void updateAdafruitIO() {
    io.run();

    static aio_status_t lastStatus = AIO_IDLE;
    aio_status_t currentStatus = io.status();
    // print when status changes
    if (currentStatus != lastStatus) {
        Serial.print("Adafruit IO status: ");
        Serial.println(io.statusText());
        lastStatus = currentStatus;
    }
}

bool adafruitIOConnected() {
    return io.status() == AIO_CONNECTED;
}

bool isDhtSensorEnabled() {
    return dhtSensorEnabled;
}

bool isSoilSensorEnabled() {
    return soilSensorEnabled;
}

bool sendToAdafruitIO(float temperatureC,
                      float humidity,
                      bool dhtEnabled,
                      bool dhtOk,
                      int soilRaw,
                      int soilPercent,
                      bool soilEnabled) {
    updateAdafruitIO();

    if (!adafruitIOConnected()) {
        Serial.println("Adafruit IO not connected, skip upload.");
        return false;
    }

    bool sentAnyData = false;

    if (dhtEnabled && dhtOk) {
        temperatureFeed->save(temperatureC);
        humidityFeed->save(humidity);
        sentAnyData = true;
    }

    if (soilEnabled) {
        soilRawFeed->save(soilRaw);
        soilMoistFeed->save(soilPercent);
        sentAnyData = true;
    }

    if (sentAnyData) {
        Serial.println("Data sent to Adafruit IO.");
    } else {
        Serial.println("No enabled sensor data to send.");
    }

    return sentAnyData;
}