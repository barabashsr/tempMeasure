# Non-Functional Requirements

## NFR1: Performance
- MQTT operations: <5% CPU overhead
- MQTT logging when enabled: Additional <2% CPU for SD writes
- Command response: <500ms
- Memory usage: <30KB additional for MQTT
- Telemetry processing: <100ms
- Message delivery: 99.9% reliability (QoS 1)

## NFR2: Compatibility
- Maintain all existing functionality
- No changes to hardware pin definitions
- Preserve Modbus register map
- Keep web API endpoints

## NFR3: Reliability
- System uptime: 99.9% minimum
- Automatic recovery from network failures
- Graceful degradation without MQTT
- Configuration persistence across restarts

## NFR4: Security
- MQTT authentication required
- Optional TLS/SSL support
- Rate limiting for commands
- Audit logging for configuration changes

## NFR5: Usability
- QR code readable from 30cm distance
- Russian translations professionally reviewed
- Intuitive command structure
- Clear error messages
