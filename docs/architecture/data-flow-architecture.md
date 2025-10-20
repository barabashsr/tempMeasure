# Data Flow Architecture

## Temperature Data Flow
```
Sensors ──▶ MeasurementPoint ──▶ TemperatureController
                                          │
                    ┌─────────────────────┼─────────────────────┐
                    ▼                     ▼                     ▼
               RegisterMap          MQTTManager            WebServer
               (Modbus)             (Remote)               (Local)
```

## Alarm Flow
```
MeasurementPoint ──▶ Alarm (State Change) ──▶ TemperatureController
                                                       │
                  ┌────────────────────────────────────┼────────────────────────┐
                  ▼                                    ▼                        ▼
          IndicatorInterface                    MQTTManager              LoggerManager
          (LED/Display/Relay)                   (Publish)                (SD Card)
```

## Command Flow
```
MQTT Broker ──▶ MQTTManager ──▶ Command Parser ──▶ TemperatureController
                     │                                      │
                     └──────────── Response ◀──────────────┘
```
