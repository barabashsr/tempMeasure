/**
 * @file test_circular_scroll.cpp
 * @brief Test program for circular text scrolling on OLED display
 * @details Tests various text lengths to verify circular scrolling implementation
 */

#include <Arduino.h>
#include <Wire.h>
#include "IndicatorInterface.h"

// Test configuration
#define PCF8575_ADDRESS 0x20
#define INTERRUPT_PIN -1

IndicatorInterface* indicator;
unsigned long lastUpdate = 0;
int testPhase = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Circular Scrolling Test Starting...");
    
    // Initialize I2C
    Wire.begin();
    
    // Create indicator interface
    indicator = new IndicatorInterface(Wire, PCF8575_ADDRESS, INTERRUPT_PIN);
    
    if (!indicator->begin()) {
        Serial.println("Failed to initialize IndicatorInterface!");
        while (1) delay(100);
    }
    
    // Configure display for testing
    indicator->setOledMode(3);  // 3 lines mode
    indicator->setOLEDOn();
    
    Serial.println("Setup complete. Starting tests...");
}

void loop() {
    // Update indicator to handle scrolling
    indicator->update();
    
    // Change test content every 10 seconds
    if (millis() - lastUpdate > 10000) {
        lastUpdate = millis();
        
        String testLines[3];
        
        switch (testPhase) {
            case 0:
                // Test 1: Short text (no scrolling)
                testLines[0] = "Short text";
                testLines[1] = "No scroll";
                testLines[2] = "Static";
                Serial.println("Test 1: Short text (no scrolling)");
                break;
                
            case 1:
                // Test 2: Medium text (slight scrolling)
                testLines[0] = "This is a medium length text line";
                testLines[1] = "Another medium line for testing";
                testLines[2] = "Third line medium length";
                Serial.println("Test 2: Medium text (slight scrolling)");
                break;
                
            case 2:
                // Test 3: Long text (significant scrolling)
                testLines[0] = "This is a very long text line that will definitely need circular scrolling to display properly";
                testLines[1] = "Temperature: 25.5°C | Humidity: 65% | Pressure: 1013.25 hPa | Status: OK";
                testLines[2] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 - Full character set test";
                Serial.println("Test 3: Long text (significant scrolling)");
                break;
                
            case 3:
                // Test 4: Mixed lengths
                testLines[0] = "Short";
                testLines[1] = "This is a very long line that demonstrates circular scrolling perfectly";
                testLines[2] = "Medium length line";
                Serial.println("Test 4: Mixed text lengths");
                break;
                
            case 4:
                // Test 5: Extreme length
                testLines[0] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua";
                testLines[1] = "!@#$%^&*()_+-={}[]|\\:;\"'<>,.?/ Special characters test with circular scrolling";
                testLines[2] = "0123456789 ".repeat(10);  // Repeated numbers
                Serial.println("Test 5: Extreme length text");
                break;
        }
        
        // Display the test lines
        indicator->printText(testLines, 3);
        
        // Move to next test phase
        testPhase++;
        if (testPhase > 4) testPhase = 0;  // Loop back to first test
    }
}