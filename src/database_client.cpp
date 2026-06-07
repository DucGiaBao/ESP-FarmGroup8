#include "database_client.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HttpClient.h>
#include "adafruit_secrets.h"

#ifndef DB_INGEST_URL
#define DB_INGEST_URL ""
#endif

#ifndef DB_WEBHOOK_SECRET
#define DB_WEBHOOK_SECRET ""
#endif

#ifndef DB_DEVICE_ID
#define DB_DEVICE_ID "esp32-yolofarm"
#endif

#ifndef DB_TLS_INSECURE
#define DB_TLS_INSECURE 1
#endif

struct DbEndpoint {
    bool valid;
    bool useTls;
    String host;
    uint16_t port;
    String path;
};

DbEndpoint dbEndpoint = {false, true, "", 443, "/"};

bool isPlaceholderValue(const char *value) {
    if (value == nullptr || value[0] == '\0') {
        return true;
    }

    if (strstr(value, "YOUR_") != nullptr) {
        return true;
    }

    return false;
}

bool parseDbIngestUrl(const char *url, DbEndpoint &endpoint) {
    String value(url);
    value.trim();

    if (value.startsWith("https://")) {
        endpoint.useTls = true;
        endpoint.port = 443;
        value.remove(0, 8);
    } else if (value.startsWith("http://")) {
        endpoint.useTls = false;
        endpoint.port = 80;
        value.remove(0, 7);
    } else {
        endpoint.valid = false;
        return false;
    }

    int slashIndex = value.indexOf('/');
    String hostPort = slashIndex >= 0 ? value.substring(0, slashIndex) : value;
    endpoint.path = slashIndex >= 0 ? value.substring(slashIndex) : "/";

    int colonIndex = hostPort.indexOf(':');
    if (colonIndex >= 0) {
        endpoint.host = hostPort.substring(0, colonIndex);
        endpoint.port = (uint16_t)hostPort.substring(colonIndex + 1).toInt();
    } else {
        endpoint.host = hostPort;
    }

    endpoint.valid = endpoint.host.length() > 0;
    return endpoint.valid;
}

bool databaseClientConfigured() {
    return !isPlaceholderValue(DB_INGEST_URL) && parseDbIngestUrl(DB_INGEST_URL, dbEndpoint);
}

void beginDatabaseClient() {
    if (databaseClientConfigured()) {
        Serial.println("Database direct upload: ENABLED");
    } else {
        Serial.println("Database direct upload: DISABLED (set DB_INGEST_URL in adafruit_secrets.h)");
    }
}

bool sendToDatabase(float temperatureC,
                    float humidity,
                    bool dhtEnabled,
                    bool dhtOk,
                    int soilRaw,
                    int soilPercent,
                    bool soilEnabled,
                    unsigned long uptimeMs) {
    if (!dbEndpoint.valid && !databaseClientConfigured()) {
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Database upload skipped: Wi-Fi is not connected.");
        return false;
    }

    WiFiClient wifiClient;
    WiFiClientSecure secureClient;
    Client *transport = &wifiClient;
    if (dbEndpoint.useTls) {
#if DB_TLS_INSECURE
        secureClient.setInsecure();
#endif
        transport = &secureClient;
    }

    HttpClient http(*transport, dbEndpoint.host.c_str(), dbEndpoint.port);
    http.setTimeout(10000);

    String payload;
    payload.reserve(384);
    payload += "{";
    payload += "\"device_id\":\"";
    payload += DB_DEVICE_ID;
    payload += "\",";
    payload += "\"uptime_ms\":";
    payload += String(uptimeMs);
    payload += ",";
    payload += "\"dht_enabled\":";
    payload += (dhtEnabled ? "true" : "false");
    payload += ",";
    payload += "\"dht_ok\":";
    payload += (dhtOk ? "true" : "false");
    payload += ",";
    payload += "\"soil_enabled\":";
    payload += (soilEnabled ? "true" : "false");
    payload += ",";

    payload += "\"temperature_c\":";
    if (dhtEnabled && dhtOk) {
        payload += String(temperatureC, 2);
    } else {
        payload += "null";
    }
    payload += ",";

    payload += "\"humidity\":";
    if (dhtEnabled && dhtOk) {
        payload += String(humidity, 2);
    } else {
        payload += "null";
    }
    payload += ",";

    payload += "\"soil_raw\":";
    if (soilEnabled) {
        payload += String(soilRaw);
    } else {
        payload += "null";
    }
    payload += ",";

    payload += "\"soil_percent\":";
    if (soilEnabled) {
        payload += String(soilPercent);
    } else {
        payload += "null";
    }
    payload += "}";

    http.beginRequest();
    http.post(dbEndpoint.path.c_str());
    http.sendHeader("Content-Type", "application/json");
    if (!isPlaceholderValue(DB_WEBHOOK_SECRET)) {
        http.sendHeader("x-webhook-secret", DB_WEBHOOK_SECRET);
    }
    http.sendHeader("Content-Length", payload.length());
    http.sendHeader("Connection", "close");
    http.beginBody();
    http.print(payload);
    http.endRequest();

    int httpCode = http.responseStatusCode();
    bool ok = false;

    if (httpCode >= 0) {
        if (httpCode >= 200 && httpCode < 300) {
            Serial.println("Data sent to database.");
            ok = true;
        } else {
            Serial.print("Database upload failed, HTTP ");
            Serial.println(httpCode);
            String response = http.responseBody();
            if (response.length() > 0) {
                Serial.println(response);
            }
        }
    } else {
        Serial.print("Database upload error: ");
        Serial.println(httpCode);
    }

    http.stop();
    return ok;
}
