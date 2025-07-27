/**
 * @file mqtt_json_test.ino
 * @brief Test sketch to verify MQTT JSON configuration structure
 * @author Claude
 * @date 2025-01-27
 * 
 * This sketch demonstrates the corrected JSON generation for MQTT configuration.
 * Upload this to your ESP32 and check the Serial Monitor output.
 */

#include <Arduino.h>
#include <ArduinoJson.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== MQTT JSON Structure Test ===\n");
    
    // Create a test configuration
    JsonDocument doc;
    
    // Basic settings
    doc["enabled"] = true;
    
    // CORRECT: Use .to<JsonObject>() to create objects
    JsonObject broker = doc["broker"].to<JsonObject>();
    broker["host"] = "987bfd99193b4a21a18a665a3812cc90.s1.eu.hivemq.cloud";
    broker["port"] = 8883;
    broker["use_tls"] = true;
    
    // INCORRECT (OLD WAY): This would create arrays
    // JsonObject broker = doc["broker"].createNestedObject();
    
    // Credentials
    JsonObject credentials = doc["credentials"].to<JsonObject>();
    credentials["username"] = "ESP32-TempCont";
    credentials["password"] = "PolzaOil2019";
    credentials["client_id"] = "";
    credentials["device_name"] = "esp32_temp_controller_01";
    
    // Topics
    JsonObject topics = doc["topics"].to<JsonObject>();
    JsonObject level2 = topics["level2"].to<JsonObject>();
    level2["type"] = "area";
    level2["value"] = "factory";
    
    // Publishing
    JsonObject publishing = doc["publishing"].to<JsonObject>();
    publishing["telemetry_interval"] = 60;
    publishing["temperature_change_threshold"] = 0.5;
    
    // Output the JSON
    Serial.println("Generated JSON:");
    serializeJsonPretty(doc, Serial);
    Serial.println("\n");
    
    // Verify structure types
    Serial.println("Structure verification:");
    Serial.print("broker is object: ");
    Serial.println(doc["broker"].is<JsonObject>() ? "✓ YES" : "✗ NO");
    Serial.print("broker is array: ");
    Serial.println(doc["broker"].is<JsonArray>() ? "✗ YES (ERROR!)" : "✓ NO (correct)");
    
    Serial.print("credentials is object: ");
    Serial.println(doc["credentials"].is<JsonObject>() ? "✓ YES" : "✗ NO");
    Serial.print("credentials is array: ");
    Serial.println(doc["credentials"].is<JsonArray>() ? "✗ YES (ERROR!)" : "✓ NO (correct)");
    
    // Test parsing back
    Serial.println("\nTesting parse back:");
    JsonVariantConst brokerRead = doc["broker"];
    if (brokerRead.is<JsonObject>()) {
        String host = brokerRead["host"] | "";
        Serial.print("Broker host: ");
        Serial.println(host);
        Serial.println(host.isEmpty() ? "✗ ERROR: Host is empty!" : "✓ Host read successfully");
    } else {
        Serial.println("✗ ERROR: Broker is not an object!");
    }
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}