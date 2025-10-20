# Business Requirements

## BR1: Remote Monitoring and Control
**Problem**: Operators need to monitor temperature data and respond to alarms without physical presence at the facility.

**Solution**: Implement MQTT protocol for:
- Real-time temperature telemetry for all 60 points
- Instant alarm notifications with priority levels
- Remote acknowledgment and control capabilities
- Integration with n8n for Telegram notifications

**Value**: Reduce response time to critical alarms by 80%, enable 24/7 monitoring without on-site staff.

## BR2: Multi-language Support
**Problem**: Russian-speaking operators struggle with English-only interface, leading to operational errors.

**Solution**: Complete Russian translation of:
- All web interface pages and elements
- Error messages and notifications
- Configuration parameters and help text

**Value**: Improve operator efficiency and reduce configuration errors by 60%.

## BR3: Quick Access to Web Interface
**Problem**: Operators waste time manually entering IP addresses, especially during commissioning or AP mode setup.

**Solution**: Display QR code on OLED containing:
- Current device IP address
- Direct URL to web interface
- AP mode connection details when applicable
- Auto-connection parameters for mobile devices

**Value**: Reduce setup time by 90%, improve field technician productivity.

## BR4: Reliable Modbus Configuration
**Problem**: Accidental configuration changes from Modbus masters during startup can corrupt alarm settings.

**Solution**: Implement explicit command trigger for register 899:
- Validate all configuration values before applying
- Require specific command sequence
- Log all Modbus configuration changes

**Value**: Prevent 100% of accidental configuration corruption incidents.
