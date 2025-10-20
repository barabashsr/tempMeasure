# Current System Architecture

## Core Components Overview
```
┌─────────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│ TemperatureController│────▶│  MeasurementPoint│────▶│      Alarm      │
│   (Orchestrator)    │     │  (60 instances)  │     │  (State Machine)│
└──────┬──────────────┘     └──────────────────┘     └─────────────────┘
       │
       ├──▶ SensorManager (DS18B20 & PT1000)
       ├──▶ ConfigManager (ConfigAssist/YAML)
       ├──▶ LoggerManager (SD Card)
       ├──▶ TempModbusServer (RTU)
       ├──▶ IndicatorInterface (OLED/LED/Button)
       └──▶ Web Server (HTML/JS/API)
```

## Key Design Patterns
- **Observer Pattern**: Alarm notifications
- **Strategy Pattern**: Sensor implementations
- **State Machine**: Alarm stage transitions
- **Manager Pattern**: Subsystem isolation
