/**
 * @file test_multiple_alarms.cpp
 * @brief Test scenarios for multiple unacknowledged alarms feature
 * @details Tests alarm queuing, priority handling, and display rotation
 */

#include <Arduino.h>
#include "TemperatureController.h"
#include "IndicatorInterface.h"
#include "MeasurementPoint.h"
#include "Alarm.h"

// Test configuration
IndicatorInterface indicator(Wire, 0x20, -1);
uint8_t oneWirePins[4] = {15, 16, 17, 18};
uint8_t csPins[4] = {5, 21, 22, 23};
TemperatureController* controller;

// Test measurement points
MeasurementPoint* testPoints[4];

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Multiple Alarms Test Starting...");
    
    // Initialize I2C and indicator
    Wire.begin();
    if (!indicator.begin()) {
        Serial.println("Failed to initialize indicator!");
        while (1) delay(100);
    }
    
    // Configure indicator ports
    indicator.setDirection(0xFFF0);  // First 4 inputs, rest outputs
    indicator.setPortName("BUTTON", 0);
    indicator.setPortName("RedLED", 4);
    indicator.setPortName("YellowLED", 5);
    indicator.setPortName("BlueLED", 6);
    indicator.setPortName("GreenLED", 7);
    indicator.setPortName("Relay1", 8);
    indicator.setPortName("Relay2", 9);
    
    // Create controller
    controller = new TemperatureController(oneWirePins, csPins, indicator);
    
    // Create test measurement points
    for (int i = 0; i < 4; i++) {
        testPoints[i] = new MeasurementPoint(i, SensorType::PT1000, 0, i);
        testPoints[i]->setName("TestPoint" + String(i));
        testPoints[i]->setHighAlarmThreshold(30.0);  // 30°C high alarm
        testPoints[i]->setLowAlarmThreshold(10.0);   // 10°C low alarm
        controller->addMeasurementPoint(testPoints[i]);
    }
    
    Serial.println("Setup complete. Starting test scenarios...");
}

void simulateTemperature(int pointIndex, float temperature) {
    testPoints[pointIndex]->setCurrentTemp(temperature);
    Serial.printf("Point %d temperature set to %.1f°C\n", pointIndex, temperature);
}

void testScenario1_MultipleHighAlarms() {
    Serial.println("\n=== Scenario 1: Multiple High Temperature Alarms ===");
    
    // Trigger high temperature alarms on multiple points
    simulateTemperature(0, 35.0);  // Critical priority
    delay(500);
    simulateTemperature(1, 32.0);  // High priority
    delay(500);
    simulateTemperature(2, 31.0);  // Medium priority
    delay(500);
    simulateTemperature(3, 30.5);  // Low priority
    
    // Set alarm priorities
    auto alarms = controller->getActiveAlarms();
    if (alarms.size() >= 4) {
        alarms[0]->setPriority(AlarmPriority::PRIORITY_CRITICAL);
        alarms[1]->setPriority(AlarmPriority::PRIORITY_HIGH);
        alarms[2]->setPriority(AlarmPriority::PRIORITY_MEDIUM);
        alarms[3]->setPriority(AlarmPriority::PRIORITY_LOW);
    }
    
    Serial.printf("Created %d active alarms\n", alarms.size());
    
    // Let the system display alarms for 20 seconds
    unsigned long testStart = millis();
    while (millis() - testStart < 20000) {
        controller->loop();
        delay(50);
    }
}

void testScenario2_AcknowledgeAlarms() {
    Serial.println("\n=== Scenario 2: Acknowledge Alarms ===");
    
    // Simulate button press to acknowledge alarms
    for (int i = 0; i < 3; i++) {
        Serial.println("Simulating button press...");
        // Note: In real implementation, you'd trigger the actual button
        // For testing, we'll directly acknowledge
        auto alarms = controller->getActiveAlarms();
        if (!alarms.empty()) {
            controller->acknowledgeHighestPriorityAlarm();
        }
        
        // Wait to see the display change
        unsigned long ackStart = millis();
        while (millis() - ackStart < 5000) {
            controller->loop();
            delay(50);
        }
    }
}

void testScenario3_MixedAlarmTypes() {
    Serial.println("\n=== Scenario 3: Mixed Alarm Types ===");
    
    // Clear previous alarms
    simulateTemperature(0, 25.0);
    simulateTemperature(1, 25.0);
    simulateTemperature(2, 25.0);
    simulateTemperature(3, 25.0);
    delay(2000);
    
    // Create different alarm types
    simulateTemperature(0, 35.0);   // High temperature
    simulateTemperature(1, 5.0);    // Low temperature
    testPoints[2]->setSensorError(true);  // Sensor error
    testPoints[3]->setDisconnected(true); // Sensor disconnected
    
    Serial.println("Created mixed alarm types");
    
    // Display for 15 seconds
    unsigned long testStart = millis();
    while (millis() - testStart < 15000) {
        controller->loop();
        delay(50);
    }
}

void testScenario4_AlarmSummary() {
    Serial.println("\n=== Scenario 4: Alarm Summary Display ===");
    
    // Test long button press for alarm summary
    Serial.println("Testing alarm summary (simulating 2-second button hold)...");
    
    // In real implementation, hold the button
    // For testing, we'll show what would be displayed
    int activeCount = 0, acknowledgedCount = 0;
    auto alarms = controller->getConfiguredAlarms();
    for (auto alarm : alarms) {
        if (alarm && alarm->isEnabled()) {
            if (alarm->getStage() == AlarmStage::ACTIVE) {
                activeCount++;
            } else if (alarm->getStage() == AlarmStage::ACKNOWLEDGED) {
                acknowledgedCount++;
            }
        }
    }
    
    Serial.printf("Alarm Summary: Active=%d, Acknowledged=%d\n", 
                  activeCount, acknowledgedCount);
    
    delay(5000);
}

void loop() {
    static int currentScenario = 1;
    static unsigned long scenarioStartTime = millis();
    
    switch (currentScenario) {
        case 1:
            if (millis() - scenarioStartTime < 100) {
                testScenario1_MultipleHighAlarms();
            }
            break;
            
        case 2:
            if (millis() - scenarioStartTime < 100) {
                testScenario2_AcknowledgeAlarms();
            }
            break;
            
        case 3:
            if (millis() - scenarioStartTime < 100) {
                testScenario3_MixedAlarmTypes();
            }
            break;
            
        case 4:
            if (millis() - scenarioStartTime < 100) {
                testScenario4_AlarmSummary();
            }
            break;
            
        default:
            Serial.println("\n=== All test scenarios completed ===");
            delay(5000);
            currentScenario = 0;  // Reset to start over
            break;
    }
    
    // Move to next scenario after delay
    if (millis() - scenarioStartTime > 30000) {
        currentScenario++;
        scenarioStartTime = millis();
    }
    
    // Always run controller loop
    controller->loop();
    delay(50);
}