# Enhanced Architecture

## Component Integration Overview
```
┌─────────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│ TemperatureController│────▶│  MeasurementPoint│────▶│      Alarm      │
│   (Orchestrator)    │     │  (60 instances)  │     │  (State Machine)│
└──────┬──────────────┘     └──────────────────┘     └─────────────────┘
       │
       ├──▶ SensorManager
       ├──▶ ConfigManager ◄─── [Extended for MQTT config]
       ├──▶ LoggerManager
       ├──▶ TempModbusServer ◄─── [Enhanced with Reg 899]
       ├──▶ IndicatorInterface ◄─── [QR Code Display]
       ├──▶ Web Server ◄─── [Russian Translation]
       └──▶ MQTTManager ◄─── [NEW Component]
```
