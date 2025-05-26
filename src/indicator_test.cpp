#include "IndicatorInterface.h"

IndicatorInterface indicator(Wire, 0x20, 34);  // I2C address 0x20, INT pin 34

void onPortChange(uint16_t currentState, uint16_t changedPins) {
    Serial.print("Ports changed: 0x");
    Serial.println(changedPins, HEX);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize I2C
    Wire.begin(21, 25);
    Wire.setClock(100000);
    
    // Initialize indicator interface
    if (!indicator.begin()) {
        Serial.println("Failed to initialize indicator interface!");
        return;
    }
    
    // Configure ports
    indicator.setDirection(0x0E00);  // P9, P10, P11 as outputs
    
    // Set port names
    indicator.setPortName("BUTTON", 8);
    indicator.setPortName("LED1", 9);
    indicator.setPortName("LED2", 10);
    indicator.setPortName("LED3", 11);
    
    // Set individual port inversion for ULN2803
    indicator.setPortInverted("LED1", false);   // Invert LED1 for ULN2803
    indicator.setPortInverted("LED2", false);   // Invert LED2 for ULN2803
    indicator.setPortInverted("LED3", false);   // Invert LED3 for ULN2803
    indicator.setPortInverted("BUTTON", false); // Button is normal (not inverted)
    
    // Or you can still use the mask method if you prefer:
    // indicator.setMode(0x0E00);  // This should now work correctly
    
    // Set interrupt callback
    indicator.setInterruptCallback(onPortChange);
    
    // Turn off all LEDs
    indicator.setAllOutputsLow();
    
    Serial.println("Setup complete!");
    indicator.printConfiguration();
}


void loop() {
    // Handle interrupts
    indicator.handleInterrupt();
    
    // Check button state
    static bool lastButtonState = true;
    bool currentButtonState = indicator.readPort("BUTTON");
    
    if (lastButtonState && !currentButtonState) {  // Button pressed
        Serial.println("Button pressed!");
        
        // Cycle LEDs
        static int ledState = 0;
        indicator.setAllOutputsLow();
        
        switch (ledState) {
            case 0: indicator.writePort("LED1", true); break;
            case 1: indicator.writePort("LED2", true); break;
            case 2: indicator.writePort("LED3", true); break;
        }
        
        ledState = (ledState + 1) % 4;
    }
    
    lastButtonState = currentButtonState;
    delay(10);
}
