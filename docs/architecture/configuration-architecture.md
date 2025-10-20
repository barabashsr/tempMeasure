# Configuration Architecture

## MQTT Configuration Storage
```cpp
struct MQTTConfig {
    char broker[128];
    uint16_t port;
    char username[64];
    char password[64];
    char clientId[64];
    uint16_t telemetryInterval;
    uint8_t qosSettings;
    bool useTLS;
    char topicPrefix[64];
};
```

## Web Interface Configuration
```html
<!-- New settings-mqtt.html -->
<div class="config-section">
    <h2 data-i18n="mqtt.title">MQTT Configuration</h2>
    <form id="mqttForm">
        <label data-i18n="mqtt.broker">Broker:</label>
        <input type="text" id="mqttBroker" maxlength="128">
        
        <label data-i18n="mqtt.port">Port:</label>
        <input type="number" id="mqttPort" min="1" max="65535">
        
        <!-- ... other fields ... -->
    </form>
</div>
```
