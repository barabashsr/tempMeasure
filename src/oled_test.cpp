#include "IndicatorInterface.h"

IndicatorInterface indicator(Wire, 0x20, -1);

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 25);
    Wire.setClock(100000);
    
    if (!indicator.begin()) {
        Serial.println("Failed to initialize!");
        return;
    }
    
    indicator.setOledMode(3);
    indicator.setOledSleepDelay(-1);
    
    // Show some text first
    String lines[] = {"Line 1", "Line 2", "Line 3"};
    indicator.printText(lines, 3);
    delay(2000);
    
    // Test blinking OK
    Serial.println("Starting OK blink...");
    indicator.blinkOK(1000);  // Blink every 1 second
}

void loop() {
    indicator.updateOLED();
    
    static unsigned long lastAction = 0;
    if (millis() - lastAction > 10000) {  // Every 10 seconds
        static bool showCross = false;
        
        if (showCross) {
            Serial.println("Starting Cross blink...");
            indicator.blinkCross(500);  // Blink every 500ms
        } else {
            Serial.println("Starting OK blink...");
            indicator.blinkCross(800);     // Blink every 800ms
        }
        
        showCross = !showCross;
        lastAction = millis();
    }
    
    delay(10);
}
