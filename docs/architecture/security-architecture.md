# Security Architecture

## MQTT Security Layers
1. **Authentication**: Username/password required
2. **Encryption**: Optional TLS/SSL support
3. **Authorization**: Read-only telemetry topics
4. **Rate Limiting**: 10 commands/minute
5. **Input Validation**: All commands validated

## Modbus Security
1. **Explicit Commands**: Register 899 triggers only
2. **Value Validation**: Range and logic checks
3. **Confirmation Sequence**: Critical operations
4. **Audit Logging**: All changes tracked
