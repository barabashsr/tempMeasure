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
    
    indicator.setOledSleepDelay(-1);
    Serial.println("Height-maximized OLED test starting...");
}

void loop() {
    static unsigned long lastModeChange = 0;
    static int currentMode = 1;
    
    indicator.updateOLED();
    
    if (millis() - lastModeChange >= 5000) {
        Serial.print("Testing ");
        Serial.print(currentMode);
        Serial.println(" line mode (height-maximized)");
        
        indicator.setOledMode(currentMode);
        String testLines[5];
        
        switch (currentMode) {
            case 1:
                testLines[0] = "БОЛЬШОЙ";  // Should fill most of the screen height
                indicator.printText(testLines, 1);
                break;
                
            case 2:
                testLines[0] = "Строка 1";
                testLines[1] = "Строка 2";
                indicator.printText(testLines, 2);
                break;
                
            case 3:
                testLines[0] = "Линия 1";
                testLines[1] = "Линия 2";
                testLines[2] = "Линия 3";
                indicator.printText(testLines, 3);
                break;
                
            case 4:
                testLines[0] = "L1: Тест";
                testLines[1] = "L2: Test";
                testLines[2] = "L3: Линия";
                testLines[3] = "L4: Line";
                indicator.printText(testLines, 4);
                break;
                
            case 5:
                testLines[0] = "1: Мини";
                testLines[1] = "2: Small";
                testLines[2] = "3: Текст";
                testLines[3] = "4: Test";
                testLines[4] = "5: Line";
                indicator.printText(testLines, 5);
                break;
        }
        
        currentMode++;
        if (currentMode > 5) {
            currentMode = 1;
            Serial.println("=== Restarting height-maximized test ===");
        }
        
        lastModeChange = millis();
    }
    
    delay(10);
}
