# Deployment Architecture

## Feature Flags
```cpp
struct FeatureFlags {
    bool mqttEnabled = false;
    bool qrCodeEnabled = true;
    bool russianEnabled = true;
    bool modbus899Enabled = false;
};
```

## Phased Rollout
1. **Phase 1**: QR code and Russian translation (low risk)
2. **Phase 2**: MQTT telemetry only (read-only)
3. **Phase 3**: MQTT commands (with monitoring)
4. **Phase 4**: Modbus 899 (after validation)

## Rollback Strategy
- Feature flags allow instant disable
- Previous firmware kept for emergency rollback
- Configuration backup before changes
- Gradual rollout to test sites first
