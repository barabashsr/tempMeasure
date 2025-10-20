# Performance Architecture

## Task Scheduling
```cpp
void TemperatureController::loop() {
    unsigned long currentMillis = millis();
    
    // High Priority (every loop)
    buttonHandler();
    alarmProcessor();
    
    // Medium Priority (100ms)
    if (currentMillis - lastDisplayUpdate > 100) {
        indicatorInterface->update();
        lastDisplayUpdate = currentMillis;
    }
    
    // Low Priority (1s)
    if (currentMillis - lastSensorRead > 1000) {
        readAllSensors();
        lastSensorRead = currentMillis;
    }
    
    // MQTT Tasks (non-blocking)
    if (mqttManager) {
        mqttManager->loop();  // Handles its own timing
    }
}
```

## MQTT Performance Optimization
- Batch temperature updates (60 points in one message)
- Delta reporting for changes only
- Command queue prevents blocking
- Async publish with callbacks
- Connection pooling for TLS
