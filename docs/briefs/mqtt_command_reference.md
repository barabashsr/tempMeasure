# MQTT Command Reference for Temperature Controller

## Table of Contents
1. [Overview](#overview)
2. [Topic Structure](#topic-structure)
3. [Message Format](#message-format)
4. [System Interrogation Commands](#system-interrogation-commands)
5. [Configuration Commands](#configuration-commands)
6. [Operational Commands](#operational-commands)
7. [Data Query Commands](#data-query-commands)
8. [Alarm Management Commands](#alarm-management-commands)
9. [Relay Control Commands](#relay-control-commands)
10. [Schedule Management Commands](#schedule-management-commands)
11. [Error Handling](#error-handling)
12. [Performance Considerations](#performance-considerations)

## Overview

This document provides a comprehensive reference for all MQTT commands available in the Temperature Controller system. The system supports 60 measurement points (50 DS18B20 + 10 PT1000) with industrial-grade monitoring and control capabilities.

### Command/Response Pattern
All commands follow a request/response pattern:
- **Request Topic**: `<prefix>/<device>/command/request`
- **Response Topic**: `<prefix>/<device>/command/response`

Each command must include a unique `cmd_id` for correlation.

## Topic Structure

### Configurable Hierarchy
```
Level 1: [Custom/Predefined] - Enterprise/Plant/Site
Level 2: [Custom/Predefined] - Area/Department/Zone  
Level 3: [Custom/Predefined] - Line/Cell/Unit
Level 4: [Device Name] - Always the device identifier
Level 5: [Topic Type] - telemetry/command/alarm/state/event/notification/schedule
Level 6: [Subtopic] - Specific data type
```

### Example Topics
```
plant/area1/line2/tempcontroller01/command/request
plant/area1/line2/tempcontroller01/command/response
plant/area1/line2/tempcontroller01/telemetry/temperature
plant/area1/line2/tempcontroller01/alarm/state
```

## Message Format

### Command Request Structure
```json
{
  "cmd_id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": "2025-01-27T10:00:00Z",
  "source": "n8n_automation",
  "command": "command_name",
  "parameters": {
    // Command-specific parameters
  }
}
```

### Command Response Structure
```json
{
  "cmd_id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": "2025-01-27T10:00:01Z",
  "command": "command_name",
  "status": "success|error",
  "execution_time": 45,  // milliseconds
  "data": {
    // Command-specific response data
  },
  "error": {  // Only present if status="error"
    "code": "ERROR_CODE",
    "message": "Human-readable error description"
  }
}
```

## System Interrogation Commands

### `get_system_info`
Get comprehensive system information including hardware, firmware, and capabilities.

**Parameters**: None

**Response**:
```json
{
  "device_id": 1000,
  "firmware_version": "2.0.0",
  "hardware_version": "1.0",
  "model": "TC-60-ESP32",
  "capabilities": {
    "max_ds18b20_sensors": 50,
    "max_pt1000_sensors": 10,
    "total_measurement_points": 60,
    "onewire_buses": 4,
    "spi_channels": 4,
    "relay_outputs": 3,
    "led_indicators": 4,
    "display_type": "OLED_128x64",
    "modbus_support": true,
    "wifi_support": true,
    "mqtt_version": "3.1.1",
    "tls_support": true
  },
  "limits": {
    "temperature_range": {
      "min": -50,
      "max": 150,
      "unit": "celsius"
    },
    "alarm_thresholds": {
      "min": -50,
      "max": 150
    },
    "measurement_period": {
      "min": 1,
      "max": 3600,
      "unit": "seconds"
    }
  }
}
```

### `get_all_points`
Retrieve complete information about all measurement points with current values and configuration.

**Parameters**: 
```json
{
  "include_unbound": false,  // Include points without sensors
  "include_config": true,    // Include alarm configuration
  "include_statistics": true // Include min/max values
}
```

**Response**:
```json
{
  "points": [
    {
      "address": 0,
      "name": "Reactor Core",
      "temperature": 75.3,
      "min_temp": 74.8,
      "max_temp": 75.9,
      "sensor": {
        "type": "DS18B20",
        "rom": "28FF1234567890AB",
        "bus": 0,
        "status": "OK"
      },
      "alarms": {
        "low_threshold": 20.0,
        "high_threshold": 80.0,
        "low_enabled": true,
        "high_enabled": true,
        "sensor_error_enabled": true,
        "hysteresis": 2.0
      },
      "status": {
        "alarm_active": false,
        "error_active": false,
        "last_update": "2025-01-27T10:00:00Z"
      }
    }
    // ... up to 59 more points
  ],
  "summary": {
    "total_points": 60,
    "bound_points": 45,
    "active_points": 43,
    "points_in_alarm": 3,
    "points_in_error": 2
  }
}
```

### `get_sensors`
Get detailed information about all discovered sensors.

**Parameters**:
```json
{
  "type": "all|DS18B20|PT1000",  // Filter by sensor type
  "status": "all|bound|unbound"   // Filter by binding status
}
```

**Response**:
```json
{
  "sensors": [
    {
      "type": "DS18B20",
      "rom": "28FF1234567890AB",
      "bus": 0,
      "bound_to": 5,
      "bound_point_name": "Heat Exchanger",
      "current_temp": 95.5,
      "status": "OK",
      "error_count": 0,
      "last_error": null
    },
    {
      "type": "PT1000",
      "chip_select": 1,
      "bound_to": 50,
      "bound_point_name": "Precision Chamber",
      "current_temp": 23.456,
      "status": "OK",
      "calibration_offset": 0.15
    }
  ],
  "summary": {
    "total_sensors": 47,
    "ds18b20_count": 45,
    "pt1000_count": 2,
    "bound_sensors": 45,
    "unbound_sensors": 2,
    "error_sensors": 0
  }
}
```

### `get_alarm_config`
Get complete alarm configuration for all points or specific point.

**Parameters**:
```json
{
  "point_address": null,  // null for all points, 0-59 for specific
  "include_relay_behavior": true,
  "include_delays": true
}
```

**Response**:
```json
{
  "global_config": {
    "hysteresis": 2.0,
    "acknowledged_delays": {
      "critical": 60000,
      "high": 300000,
      "medium": 600000,
      "low": 1800000
    },
    "relay_behavior": {
      "critical": {
        "active": "RELAY1_ON|RELAY2_ON|RED_LED|BLUE_LED",
        "acknowledged": "BLUE_LED"
      },
      "high": {
        "active": "RELAY1_ON|RED_LED",
        "acknowledged": "YELLOW_LED"
      },
      "medium": {
        "active": "YELLOW_LED",
        "acknowledged": "NONE"
      },
      "low": {
        "active": "BLINK_YELLOW",
        "acknowledged": "NONE"
      }
    }
  },
  "point_configs": [
    {
      "address": 0,
      "name": "Reactor Core",
      "alarms": {
        "low_temperature": {
          "enabled": true,
          "threshold": 20.0,
          "priority": "HIGH",
          "delay": 30
        },
        "high_temperature": {
          "enabled": true,
          "threshold": 80.0,
          "priority": "CRITICAL",
          "delay": 0
        },
        "sensor_error": {
          "enabled": true,
          "priority": "HIGH"
        }
      }
    }
  ]
}
```

### `get_modbus_map`
Get Modbus register mapping information.

**Parameters**: None

**Response**:
```json
{
  "register_map": {
    "device_info": {
      "base": 0,
      "registers": {
        "0": "Device ID (R/W)",
        "1": "Firmware Version (R)",
        "2": "Active DS18B20 Count (R)",
        "3": "Active PT1000 Count (R)",
        "4-10": "Device Status Flags (R)",
        "11-13": "Relay Status (R)"
      }
    },
    "temperatures": {
      "current": {
        "base": 100,
        "range": "100-159",
        "description": "Current temperature values (R)"
      },
      "min": {
        "base": 200,
        "range": "200-259",
        "description": "Minimum temperature values (R)"
      },
      "max": {
        "base": 300,
        "range": "300-359",
        "description": "Maximum temperature values (R)"
      }
    },
    "alarms": {
      "status": {
        "base": 400,
        "range": "400-459",
        "description": "Alarm status flags (R)"
      },
      "error_status": {
        "base": 500,
        "range": "500-559",
        "description": "Error status flags (R)"
      },
      "low_thresholds": {
        "base": 600,
        "range": "600-659",
        "description": "Low temperature thresholds (R/W)"
      },
      "high_thresholds": {
        "base": 700,
        "range": "700-759",
        "description": "High temperature thresholds (R/W)"
      }
    },
    "control": {
      "alarm_config": {
        "base": 800,
        "range": "800-859",
        "description": "Alarm enable/priority config (R/W)"
      },
      "relay_control": {
        "base": 860,
        "range": "860-865",
        "description": "Relay control/status (R/W)"
      },
      "hysteresis": {
        "base": 870,
        "range": "870-889",
        "description": "Hysteresis values (R/W)"
      },
      "command": {
        "base": 899,
        "description": "Command register (W)"
      }
    }
  }
}
```

### `get_network_config`
Get current network configuration.

**Parameters**: None

**Response**:
```json
{
  "wifi": {
    "enabled": true,
    "mode": "STA",
    "ssid": "Industrial-WiFi",
    "ip_address": "192.168.1.100",
    "subnet_mask": "255.255.255.0",
    "gateway": "192.168.1.1",
    "dns": "192.168.1.1",
    "rssi": -65,
    "mac_address": "AA:BB:CC:DD:EE:FF"
  },
  "mqtt": {
    "enabled": true,
    "broker": "broker.hivemq.com",
    "port": 1883,
    "use_tls": false,
    "client_id": "tempcontroller01",
    "connected": true,
    "messages_sent": 54321,
    "messages_received": 12345,
    "reconnects": 2
  },
  "modbus": {
    "enabled": true,
    "mode": "RTU",
    "slave_id": 1,
    "baud_rate": 9600,
    "parity": "NONE",
    "stop_bits": 1,
    "requests_processed": 12345
  }
}
```

### `get_time_config`
Get RTC and time synchronization status.

**Parameters**: None

**Response**:
```json
{
  "current_time": "2025-01-27T10:00:00Z",
  "timezone": "UTC+3",
  "rtc": {
    "present": true,
    "type": "DS3231",
    "battery_ok": true,
    "temperature": 25.5
  },
  "ntp": {
    "enabled": true,
    "server": "pool.ntp.org",
    "sync_interval": 3600,
    "last_sync": "2025-01-27T09:00:00Z",
    "sync_status": "OK"
  },
  "uptime": {
    "seconds": 432000,
    "formatted": "5d 0h 0m 0s"
  }
}
```

### `get_logging_config`
Get data logging configuration and status.

**Parameters**: None

**Response**:
```json
{
  "temperature_logging": {
    "enabled": true,
    "interval": 300,  // seconds
    "storage": "SD_CARD",
    "format": "CSV",
    "current_file": "temp_log_2025-01-27_0.csv",
    "file_size": 524288,
    "free_space": 15728640,
    "rotation": "DAILY",
    "retention_days": 30
  },
  "event_logging": {
    "enabled": true,
    "storage": "SD_CARD",
    "current_file": "event_log_2025-01-27.csv",
    "types": ["ALARM", "CONFIG_CHANGE", "SENSOR_ERROR", "SYSTEM"]
  },
  "alarm_state_logging": {
    "enabled": true,
    "storage": "SD_CARD",
    "current_file": "alarm_states_2025-01-27.csv"
  }
}
```

### `get_display_status`
Get OLED display status and configuration.

**Parameters**: None

**Response**:
```json
{
  "display": {
    "type": "OLED_128x64",
    "enabled": true,
    "brightness": 128,
    "contrast": 255,
    "rotation": 0,
    "current_mode": "ALARM_ACTIVE",
    "timeout": 10000,
    "last_activity": "2025-01-27T09:59:50Z"
  },
  "current_content": {
    "mode": "ALARM",
    "line1": "HIGH TEMP ALARM",
    "line2": "Point 5: 95.5°C",
    "line3": "Threshold: 90.0°C",
    "priority": "CRITICAL"
  }
}
```

## Configuration Commands

### `set_point_config`
Configure a measurement point.

**Parameters**:
```json
{
  "point_address": 0,  // 0-59
  "config": {
    "name": "New Point Name",
    "low_threshold": -10.0,
    "high_threshold": 50.0,
    "low_priority": "MEDIUM",
    "high_priority": "HIGH",
    "sensor_error_priority": "CRITICAL",
    "low_enabled": true,
    "high_enabled": true,
    "sensor_error_enabled": true,
    "hysteresis": 2.0,
    "alarm_delay": 30  // seconds
  }
}
```

**Response**:
```json
{
  "point_address": 0,
  "previous_config": { /* old configuration */ },
  "new_config": { /* applied configuration */ },
  "validation_warnings": []
}
```

**Validation**:
- Temperature thresholds must be within system limits (-50 to 150°C)
- Low threshold must be less than high threshold
- Hysteresis must be positive (0.1 to 10.0)
- Alarm delay must be 0-3600 seconds

### `set_alarm_thresholds`
Bulk update alarm thresholds for multiple points.

**Parameters**:
```json
{
  "updates": [
    {
      "point_address": 0,
      "low_threshold": 15.0,
      "high_threshold": 85.0
    },
    {
      "point_address": 5,
      "low_threshold": 20.0,
      "high_threshold": 90.0
    }
  ],
  "apply_hysteresis": 2.0  // Optional, applies to all
}
```

**Response**:
```json
{
  "updated": 2,
  "failed": 0,
  "results": [
    {
      "point_address": 0,
      "status": "success"
    },
    {
      "point_address": 5,
      "status": "success"
    }
  ]
}
```

### `set_measurement_period`
Set the global measurement period.

**Parameters**:
```json
{
  "period_seconds": 10,  // 1-3600
  "apply_immediately": true
}
```

**Response**:
```json
{
  "previous_period": 5,
  "new_period": 10,
  "next_measurement": "2025-01-27T10:00:10Z"
}
```

### `set_modbus_config`
Configure Modbus parameters.

**Parameters**:
```json
{
  "enabled": true,
  "slave_id": 1,  // 1-247
  "baud_rate": 9600,  // 9600, 19200, 38400, 57600, 115200
  "parity": "NONE",  // NONE, EVEN, ODD
  "stop_bits": 1,  // 1, 2
  "timeout": 1000  // milliseconds
}
```

**Response**:
```json
{
  "status": "success",
  "restart_required": true,
  "message": "Modbus configuration updated. Restart required to apply changes."
}
```

### `set_network_config`
Configure network settings.

**Parameters**:
```json
{
  "wifi": {
    "enabled": true,
    "ssid": "NewNetwork",
    "password": "SecurePassword123",
    "dhcp": true,
    "static_ip": null,
    "static_gateway": null,
    "static_subnet": null,
    "static_dns": null
  },
  "mqtt": {
    "enabled": true,
    "broker": "mqtt.industrial.com",
    "port": 8883,
    "use_tls": true,
    "username": "device01",
    "password": "mqttpass",
    "client_id": "tempcontroller01",
    "keepalive": 60,
    "qos_telemetry": 1,
    "qos_alarms": 2,
    "qos_commands": 1
  }
}
```

**Response**:
```json
{
  "wifi": {
    "status": "success",
    "reconnect_required": true
  },
  "mqtt": {
    "status": "success",
    "reconnect_initiated": true
  }
}
```

### `set_relay_mode`
Configure relay control mode.

**Parameters**:
```json
{
  "relay_number": 1,  // 1-3
  "mode": "AUTO",  // AUTO, FORCE_ON, FORCE_OFF
  "state": null  // Only used when mode is FORCE_ON/FORCE_OFF
}
```

**Response**:
```json
{
  "relay_number": 1,
  "previous_mode": "AUTO",
  "new_mode": "FORCE_ON",
  "current_state": true,
  "commanded_state": true
}
```

### `set_logging_config`
Configure data logging parameters.

**Parameters**:
```json
{
  "temperature_logging": {
    "enabled": true,
    "interval": 300,  // seconds
    "format": "CSV",
    "rotation": "DAILY",  // DAILY, SIZE, NONE
    "max_file_size": 10485760,  // bytes (for SIZE rotation)
    "retention_days": 30
  },
  "event_logging": {
    "enabled": true,
    "types": ["ALARM", "CONFIG_CHANGE", "SENSOR_ERROR"]
  },
  "alarm_state_logging": {
    "enabled": true
  }
}
```

**Response**:
```json
{
  "status": "success",
  "new_files_created": ["temp_log_2025-01-27_1.csv"],
  "storage_check": {
    "free_space": 15728640,
    "estimated_days": 45
  }
}
```

### `set_display_config`
Configure OLED display settings.

**Parameters**:
```json
{
  "enabled": true,
  "brightness": 128,  // 0-255
  "contrast": 255,  // 0-255
  "rotation": 0,  // 0, 90, 180, 270
  "timeout": 10000,  // milliseconds, 0 to disable
  "show_ip_on_startup": true,
  "alarm_scroll_speed": 1000  // milliseconds
}
```

**Response**:
```json
{
  "status": "success",
  "display_test": "OK"
}
```

## Operational Commands

### `force_measurement`
Trigger immediate temperature measurement.

**Parameters**:
```json
{
  "points": "all",  // "all" or array of addresses [0, 5, 10]
  "skip_schedule": true  // Don't wait for next scheduled measurement
}
```

**Response**:
```json
{
  "measurements_triggered": 60,
  "measurements_completed": 58,
  "measurements_failed": 2,
  "duration_ms": 850,
  "failed_points": [
    {
      "address": 15,
      "reason": "SENSOR_ERROR"
    },
    {
      "address": 32,
      "reason": "NO_SENSOR_BOUND"
    }
  ]
}
```

### `reset_min_max`
Reset min/max values for measurement points.

**Parameters**:
```json
{
  "points": "all",  // "all" or array of addresses
  "confirm": true  // Safety confirmation
}
```

**Response**:
```json
{
  "points_reset": 60,
  "timestamp": "2025-01-27T10:00:00Z"
}
```

### `clear_alarm_history`
Clear alarm history data.

**Parameters**:
```json
{
  "date_range": {
    "start": "2025-01-01",
    "end": "2025-01-26"
  },
  "alarm_types": ["HIGH_TEMPERATURE", "LOW_TEMPERATURE"],
  "points": "all",
  "confirm": true
}
```

**Response**:
```json
{
  "records_deleted": 1523,
  "files_affected": [
    "alarm_states_2025-01-15.csv",
    "alarm_states_2025-01-20.csv"
  ],
  "space_freed": 524288
}
```

### `export_config`
Export complete system configuration.

**Parameters**:
```json
{
  "format": "JSON",  // JSON, CSV, BINARY
  "include": {
    "points": true,
    "alarms": true,
    "network": true,
    "modbus": true,
    "logging": true,
    "display": true
  },
  "password_protect": false
}
```

**Response**:
```json
{
  "export_id": "config_20250127_100000",
  "format": "JSON",
  "size": 16384,
  "checksum": "sha256:1234567890abcdef",
  "download_url": "/api/download/config_20250127_100000.json",
  "expires": "2025-01-27T11:00:00Z",
  "data": {
    // Complete configuration object if size < 64KB
  }
}
```

### `import_config`
Import system configuration.

**Parameters**:
```json
{
  "source": "inline",  // inline, url, file_id
  "data": { /* configuration object */ },
  "url": null,
  "file_id": null,
  "merge_mode": "replace",  // replace, merge, update
  "validate_only": false,
  "backup_current": true
}
```

**Response**:
```json
{
  "validation": {
    "valid": true,
    "warnings": [
      "Point 45 sensor binding will be cleared (sensor not found)"
    ],
    "errors": []
  },
  "import_status": "success",
  "backup_id": "backup_20250127_095950",
  "changes": {
    "points_modified": 15,
    "alarms_modified": 45,
    "settings_modified": 3
  },
  "restart_required": false
}
```

### `self_test`
Run comprehensive system self-test.

**Parameters**:
```json
{
  "tests": {
    "sensors": true,
    "memory": true,
    "storage": true,
    "network": true,
    "display": true,
    "rtc": true,
    "relays": true,
    "leds": true
  },
  "duration": "full"  // quick, normal, full
}
```

**Response**:
```json
{
  "test_id": "test_20250127_100000",
  "duration_ms": 15230,
  "overall_status": "PASS_WITH_WARNINGS",
  "results": {
    "sensors": {
      "status": "PASS",
      "details": {
        "ds18b20_found": 45,
        "ds18b20_responsive": 45,
        "pt1000_found": 2,
        "pt1000_responsive": 2,
        "buses_tested": 4,
        "bus_errors": 0
      }
    },
    "memory": {
      "status": "WARNING",
      "details": {
        "heap_free": 45678,
        "heap_fragmentation": 15,
        "stack_high_water": 1024,
        "psram_available": false
      }
    },
    "storage": {
      "status": "PASS",
      "details": {
        "sd_card_present": true,
        "sd_card_size": 32212254720,
        "sd_card_free": 15728640000,
        "write_test": "OK",
        "read_test": "OK",
        "speed_mbps": 18.5
      }
    },
    "network": {
      "status": "PASS",
      "details": {
        "wifi_connected": true,
        "wifi_rssi": -65,
        "ping_gateway": "OK",
        "ping_dns": "OK",
        "mqtt_connected": true
      }
    },
    "display": {
      "status": "PASS",
      "details": {
        "type": "OLED_128x64",
        "communication": "OK",
        "pixel_test": "OK"
      }
    },
    "rtc": {
      "status": "PASS",
      "details": {
        "present": true,
        "battery_voltage": 3.1,
        "time_drift_seconds": 2
      }
    },
    "relays": {
      "status": "PASS",
      "details": {
        "relay1": "OK",
        "relay2": "OK",
        "relay3": "OK"
      }
    },
    "leds": {
      "status": "PASS",
      "details": {
        "red": "OK",
        "yellow": "OK",
        "green": "OK",
        "blue": "OK"
      }
    }
  },
  "recommendations": [
    "Consider increasing heap size - fragmentation at 15%",
    "SD card 49% full - consider archiving old logs"
  ]
}
```

### `restart_system`
Restart the temperature controller.

**Parameters**:
```json
{
  "delay_seconds": 5,  // 0-60
  "save_state": true,
  "reason": "Configuration change"
}
```

**Response**:
```json
{
  "restart_scheduled": "2025-01-27T10:00:05Z",
  "state_saved": true,
  "uptime_before_restart": 432000
}
```

### `factory_reset`
Reset system to factory defaults.

**Parameters**:
```json
{
  "confirm": "FACTORY_RESET",  // Must match exactly
  "preserve": {
    "network": false,
    "device_id": true
  }
}
```

**Response**:
```json
{
  "status": "success",
  "backup_created": "backup_before_reset_20250127_100000",
  "restart_required": true,
  "restart_in_seconds": 10
}
```

## Data Query Commands

### `get_historical_data`
Retrieve historical temperature data.

**Parameters**:
```json
{
  "points": [0, 5, 10],  // Array of point addresses or "all"
  "time_range": {
    "start": "2025-01-26T00:00:00Z",
    "end": "2025-01-27T00:00:00Z"
  },
  "resolution": "raw",  // raw, 1min, 5min, 15min, 1hour
  "format": "json",  // json, csv
  "include_statistics": true
}
```

**Response**:
```json
{
  "query_id": "hist_20250127_100000",
  "time_range": {
    "start": "2025-01-26T00:00:00Z",
    "end": "2025-01-27T00:00:00Z"
  },
  "resolution": "5min",
  "data_points": 864,  // 288 samples * 3 points
  "data": [
    {
      "timestamp": "2025-01-26T00:00:00Z",
      "points": {
        "0": 23.5,
        "5": 45.2,
        "10": 67.8
      }
    },
    // ... more data points
  ],
  "statistics": {
    "0": {
      "min": 22.1,
      "max": 24.8,
      "avg": 23.5,
      "std_dev": 0.8,
      "samples": 288
    },
    "5": {
      "min": 44.0,
      "max": 46.5,
      "avg": 45.2,
      "std_dev": 0.6,
      "samples": 288
    },
    "10": {
      "min": 65.5,
      "max": 69.2,
      "avg": 67.8,
      "std_dev": 1.1,
      "samples": 288
    }
  }
}
```

### `get_alarm_history`
Retrieve alarm history with filtering options.

**Parameters**:
```json
{
  "filters": {
    "points": "all",  // "all" or array of addresses
    "types": ["HIGH_TEMPERATURE", "SENSOR_ERROR"],
    "priorities": ["CRITICAL", "HIGH"],
    "date_range": {
      "start": "2025-01-20",
      "end": "2025-01-27"
    },
    "include_acknowledged": true,
    "include_resolved": true
  },
  "sort": "timestamp_desc",  // timestamp_asc, timestamp_desc, priority, duration
  "limit": 100,
  "offset": 0
}
```

**Response**:
```json
{
  "total_records": 523,
  "returned_records": 100,
  "has_more": true,
  "alarms": [
    {
      "alarm_id": "ALM_2025_0127_001",
      "point_address": 5,
      "point_name": "Heat Exchanger",
      "type": "HIGH_TEMPERATURE",
      "priority": "CRITICAL",
      "triggered_at": "2025-01-27T08:30:15Z",
      "acknowledged_at": "2025-01-27T08:31:00Z",
      "resolved_at": "2025-01-27T08:45:30Z",
      "duration_seconds": 915,
      "peak_value": 95.5,
      "threshold": 90.0,
      "trigger_count": 1,
      "notes": "Cooling pump failure"
    },
    // ... more records
  ],
  "summary": {
    "by_type": {
      "HIGH_TEMPERATURE": 234,
      "LOW_TEMPERATURE": 89,
      "SENSOR_ERROR": 200
    },
    "by_priority": {
      "CRITICAL": 45,
      "HIGH": 178,
      "MEDIUM": 267,
      "LOW": 33
    },
    "average_duration": 485,
    "longest_duration": 7200
  }
}
```

### `get_statistics`
Get system operation statistics.

**Parameters**:
```json
{
  "period": "today",  // today, week, month, all
  "categories": ["temperature", "alarms", "sensors", "system"]
}
```

**Response**:
```json
{
  "period": {
    "start": "2025-01-27T00:00:00Z",
    "end": "2025-01-27T10:00:00Z"
  },
  "temperature": {
    "global_min": 15.2,
    "global_min_point": 12,
    "global_min_time": "2025-01-27T04:23:00Z",
    "global_max": 95.5,
    "global_max_point": 5,
    "global_max_time": "2025-01-27T08:30:15Z",
    "average_all_points": 45.3,
    "measurement_count": 72000  // 10 hours * 60 points * 12/hour
  },
  "alarms": {
    "total_triggered": 45,
    "by_type": {
      "HIGH_TEMPERATURE": 15,
      "LOW_TEMPERATURE": 5,
      "SENSOR_ERROR": 25
    },
    "by_priority": {
      "CRITICAL": 3,
      "HIGH": 12,
      "MEDIUM": 25,
      "LOW": 5
    },
    "average_response_time": 45,  // seconds to acknowledge
    "unacknowledged": 2
  },
  "sensors": {
    "total_errors": 156,
    "errors_by_type": {
      "CRC_ERROR": 100,
      "TIMEOUT": 45,
      "DISCONNECTED": 11
    },
    "most_unreliable": [
      {
        "point": 32,
        "error_count": 45,
        "error_rate": 0.0625  // 6.25%
      }
    ],
    "sensor_replacements": 0
  },
  "system": {
    "uptime_seconds": 432000,
    "restarts": 0,
    "cpu_load_avg": 15.5,
    "memory_free_avg": 45678,
    "mqtt_messages_sent": 54321,
    "mqtt_messages_received": 12345,
    "modbus_requests_handled": 98765,
    "config_changes": 12
  }
}
```

### `get_trend_analysis`
Get temperature trend analysis for specific points.

**Parameters**:
```json
{
  "points": [0, 5, 10],
  "time_window": "24h",  // 1h, 6h, 24h, 7d, 30d
  "analysis_type": ["rate_of_change", "prediction", "anomalies"]
}
```

**Response**:
```json
{
  "time_window": {
    "start": "2025-01-26T10:00:00Z",
    "end": "2025-01-27T10:00:00Z"
  },
  "analysis": {
    "0": {
      "current_temp": 23.5,
      "rate_of_change": {
        "per_minute": 0.02,
        "per_hour": 1.2,
        "trend": "increasing"
      },
      "prediction": {
        "next_hour": 24.7,
        "confidence": 0.85
      },
      "anomalies": []
    },
    "5": {
      "current_temp": 95.5,
      "rate_of_change": {
        "per_minute": -0.15,
        "per_hour": -9.0,
        "trend": "decreasing"
      },
      "prediction": {
        "next_hour": 86.5,
        "confidence": 0.92
      },
      "anomalies": [
        {
          "timestamp": "2025-01-27T08:30:00Z",
          "type": "spike",
          "severity": "high",
          "value": 95.5,
          "expected_range": [85.0, 90.0]
        }
      ]
    }
  }
}
```

### `bulk_export`
Export large datasets for external analysis.

**Parameters**:
```json
{
  "data_type": "temperature",  // temperature, alarms, events, all
  "format": "csv",  // csv, json, parquet
  "compression": "gzip",  // none, gzip, zip
  "time_range": {
    "start": "2025-01-01",
    "end": "2025-01-27"
  },
  "delivery": "download",  // download, mqtt_chunked, webhook
  "webhook_url": null
}
```

**Response**:
```json
{
  "export_id": "bulk_20250127_100000",
  "status": "processing",
  "estimated_size": 52428800,  // 50MB
  "estimated_time": 30,  // seconds
  "progress_topic": "plant/area1/line2/tempcontroller01/export/progress",
  "completion_topic": "plant/area1/line2/tempcontroller01/export/complete"
}
```

## Alarm Management Commands

### `acknowledge_alarm`
Acknowledge specific alarm by ID or point.

**Parameters**:
```json
{
  "alarm_id": "ALM_2025_0127_001",  // Or use point_address + type
  "point_address": null,
  "alarm_type": null,
  "note": "Maintenance crew dispatched"
}
```

**Response**:
```json
{
  "alarm_id": "ALM_2025_0127_001",
  "previous_state": "ACTIVE",
  "new_state": "ACKNOWLEDGED",
  "acknowledged_by": "mqtt_user",
  "acknowledged_at": "2025-01-27T10:00:00Z",
  "auto_resolve_at": "2025-01-27T10:05:00Z"
}
```

### `acknowledge_all_alarms`
Acknowledge all active alarms.

**Parameters**:
```json
{
  "filter": {
    "priorities": ["CRITICAL", "HIGH"],  // Optional filter
    "types": null,  // Optional filter
    "points": null  // Optional filter
  },
  "note": "Shift change acknowledgment"
}
```

**Response**:
```json
{
  "acknowledged_count": 5,
  "alarms": [
    {
      "alarm_id": "ALM_2025_0127_001",
      "point": 5,
      "type": "HIGH_TEMPERATURE",
      "priority": "CRITICAL"
    },
    // ... more alarms
  ]
}
```

### `test_alarm`
Trigger test alarm for validation.

**Parameters**:
```json
{
  "point_address": 0,
  "alarm_type": "HIGH_TEMPERATURE",  // HIGH_TEMPERATURE, LOW_TEMPERATURE, SENSOR_ERROR
  "duration_seconds": 60,  // How long to maintain test condition
  "value": 95.0  // Simulated value
}
```

**Response**:
```json
{
  "test_id": "test_alarm_20250127_100000",
  "point_address": 0,
  "alarm_triggered": true,
  "alarm_id": "ALM_TEST_2025_0127_001",
  "outputs_activated": ["RELAY1", "RED_LED", "BLUE_LED"],
  "cancel_topic": "plant/area1/line2/tempcontroller01/command/cancel_test"
}
```

### `set_alarm_priority`
Change alarm priority for a point.

**Parameters**:
```json
{
  "point_address": 5,
  "alarm_type": "HIGH_TEMPERATURE",
  "new_priority": "CRITICAL",  // LOW, MEDIUM, HIGH, CRITICAL
  "apply_to_active": true  // Update currently active alarm
}
```

**Response**:
```json
{
  "point_address": 5,
  "alarm_type": "HIGH_TEMPERATURE",
  "previous_priority": "HIGH",
  "new_priority": "CRITICAL",
  "active_alarm_updated": true
}
```

### `mute_alarms`
Temporarily mute alarm outputs.

**Parameters**:
```json
{
  "duration_minutes": 30,  // 0 for indefinite
  "mute_outputs": ["RELAY", "BUZZER"],  // RELAY, LED, BUZZER, DISPLAY
  "keep_logging": true
}
```

**Response**:
```json
{
  "mute_active": true,
  "mute_until": "2025-01-27T10:30:00Z",
  "muted_outputs": ["RELAY", "BUZZER"],
  "active_alarms_count": 3
}
```

## Relay Control Commands

### `get_relay_status`
Get current relay status and configuration.

**Parameters**: None

**Response**:
```json
{
  "relays": {
    "1": {
      "mode": "AUTO",
      "commanded_state": true,
      "actual_state": true,
      "auto_control": "CRITICAL_ALARM",
      "last_change": "2025-01-27T08:30:15Z"
    },
    "2": {
      "mode": "FORCE_OFF",
      "commanded_state": false,
      "actual_state": false,
      "auto_control": "HIGH_ALARM",
      "last_change": "2025-01-27T07:00:00Z"
    },
    "3": {
      "mode": "AUTO",
      "commanded_state": false,
      "actual_state": false,
      "auto_control": "MODBUS_ONLY",
      "last_change": null
    }
  }
}
```

### `control_relay`
Direct relay control.

**Parameters**:
```json
{
  "relay_number": 1,  // 1-3
  "action": "toggle",  // on, off, toggle, pulse
  "pulse_duration_ms": 1000,  // Only for pulse action
  "override_mode": true  // Force action regardless of mode
}
```

**Response**:
```json
{
  "relay_number": 1,
  "action_executed": "toggle",
  "previous_state": true,
  "new_state": false,
  "mode": "FORCE_OFF",
  "warning": null
}
```

### `set_relay_behavior`
Configure relay behavior for alarm priorities.

**Parameters**:
```json
{
  "priority": "CRITICAL",
  "active_behavior": {
    "relay1": true,
    "relay2": true,
    "relay3": false
  },
  "acknowledged_behavior": {
    "relay1": false,
    "relay2": true,
    "relay3": false
  }
}
```

**Response**:
```json
{
  "priority": "CRITICAL",
  "previous_config": { /* old configuration */ },
  "new_config": { /* new configuration */ },
  "affected_alarms": 2
}
```

## Schedule Management Commands

### `add_schedule`
Add a scheduled command execution.

**Parameters**:
```json
{
  "schedule_id": "daily_report",
  "description": "Daily temperature summary",
  "schedule_type": "cron",  // interval, cron, once
  "cron_expression": "0 8 * * *",  // For cron type
  "interval_seconds": null,  // For interval type
  "once_at": null,  // For once type
  "command": "get_statistics",
  "parameters": {
    "period": "today",
    "categories": ["temperature", "alarms"]
  },
  "enabled": true,
  "publish_topic": "plant/area1/reports/daily"
}
```

**Response**:
```json
{
  "schedule_id": "daily_report",
  "created": true,
  "next_execution": "2025-01-28T08:00:00Z",
  "test_result": {
    "valid": true,
    "output_size": 2048
  }
}
```

### `list_schedules`
List all configured schedules.

**Parameters**:
```json
{
  "filter": {
    "enabled": null,  // true, false, or null for all
    "command": null  // Filter by command name
  }
}
```

**Response**:
```json
{
  "schedules": [
    {
      "id": "daily_report",
      "description": "Daily temperature summary",
      "type": "cron",
      "expression": "0 8 * * *",
      "command": "get_statistics",
      "enabled": true,
      "last_run": "2025-01-27T08:00:00Z",
      "last_status": "success",
      "next_run": "2025-01-28T08:00:00Z",
      "run_count": 30
    },
    {
      "id": "high_freq_monitor",
      "description": "High frequency point monitoring",
      "type": "interval",
      "interval_seconds": 60,
      "command": "get_point",
      "parameters": {
        "point_id": 5,
        "include_history": true,
        "history_minutes": 5
      },
      "enabled": true,
      "last_run": "2025-01-27T09:59:00Z",
      "last_status": "success",
      "next_run": "2025-01-27T10:00:00Z",
      "run_count": 1440
    }
  ],
  "summary": {
    "total": 2,
    "enabled": 2,
    "disabled": 0
  }
}
```

### `update_schedule`
Modify existing schedule.

**Parameters**:
```json
{
  "schedule_id": "daily_report",
  "updates": {
    "enabled": false,
    "cron_expression": "0 9 * * *",  // Change to 9 AM
    "parameters": {
      "period": "today",
      "categories": ["temperature", "alarms", "sensors"]
    }
  }
}
```

**Response**:
```json
{
  "schedule_id": "daily_report",
  "updated": true,
  "changes": ["enabled", "cron_expression", "parameters"],
  "next_execution": null  // null because disabled
}
```

### `delete_schedule`
Remove a schedule.

**Parameters**:
```json
{
  "schedule_id": "high_freq_monitor",
  "confirm": true
}
```

**Response**:
```json
{
  "schedule_id": "high_freq_monitor",
  "deleted": true,
  "was_enabled": true,
  "total_runs": 1440
}
```

### `run_schedule_now`
Execute a scheduled command immediately.

**Parameters**:
```json
{
  "schedule_id": "daily_report",
  "skip_next": false  // Skip next scheduled execution
}
```

**Response**:
```json
{
  "schedule_id": "daily_report",
  "execution_id": "exec_20250127_100000",
  "status": "success",
  "duration_ms": 125,
  "output_size": 2048,
  "published_to": "plant/area1/reports/daily"
}
```

## Error Handling

### Standard Error Response
All commands return errors in a consistent format:

```json
{
  "cmd_id": "550e8400-e29b-41d4-a716-446655440000",
  "timestamp": "2025-01-27T10:00:01Z",
  "command": "set_point_config",
  "status": "error",
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Low threshold (90.0) must be less than high threshold (80.0)",
    "field": "low_threshold",
    "details": {
      "point_address": 5,
      "low_threshold": 90.0,
      "high_threshold": 80.0
    }
  }
}
```

### Common Error Codes

| Code | Description | Recovery Action |
|------|-------------|-----------------|
| `INVALID_COMMAND` | Unknown command name | Check command spelling |
| `INVALID_PARAMETERS` | Missing or invalid parameters | Review parameter requirements |
| `VALIDATION_ERROR` | Parameter validation failed | Check parameter constraints |
| `POINT_NOT_FOUND` | Invalid point address | Use valid address 0-59 |
| `SENSOR_NOT_FOUND` | Referenced sensor doesn't exist | Check sensor discovery |
| `PERMISSION_DENIED` | Insufficient privileges | Check MQTT user permissions |
| `RESOURCE_BUSY` | Resource is currently busy | Retry after delay |
| `TIMEOUT` | Command execution timeout | Retry or check system status |
| `STORAGE_FULL` | Storage space exhausted | Clear old logs/data |
| `HARDWARE_ERROR` | Hardware component failure | Check self-test results |
| `CONFIGURATION_LOCKED` | Configuration is locked | Unlock via physical access |

## Performance Considerations

### Command Timing
- **Fast Commands** (<50ms): Status queries, configuration reads
- **Medium Commands** (50-200ms): Configuration writes, single point operations
- **Slow Commands** (200ms-2s): Bulk operations, sensor discovery, self-test
- **Very Slow Commands** (>2s): Historical data queries, bulk exports

### Rate Limiting
- **Status Commands**: No limit
- **Configuration Commands**: 10 per minute
- **Bulk Operations**: 1 per minute
- **System Commands**: 1 per 5 minutes

### Best Practices

1. **Use Bulk Commands**: When updating multiple points, use bulk commands instead of individual updates
2. **Subscribe to Telemetry**: For real-time data, subscribe to telemetry topics instead of polling
3. **Cache Configuration**: Cache configuration locally and use change notifications
4. **Respect QoS Levels**: Use appropriate QoS for command importance
5. **Handle Timeouts**: Implement timeout handling for all commands
6. **Validate Locally**: Validate parameters before sending to reduce errors
7. **Use Compression**: Enable compression for large data transfers
8. **Schedule Wisely**: Distribute scheduled commands to avoid peaks

### Memory Constraints
- Maximum command parameters size: 8KB
- Maximum response size: 64KB (larger responses use chunking)
- Maximum concurrent commands: 10
- Command queue depth: 20

### Network Optimization
- Enable MQTT persistent sessions
- Use QoS 0 for telemetry, QoS 1 for commands, QoS 2 for critical operations
- Implement local buffering during disconnections
- Use retain flag for configuration topics
- Batch small commands when possible

## Industrial Use Cases

### 1. Remote Monitoring Dashboard
```javascript
// Subscribe to telemetry
mqtt.subscribe('plant/+/+/+/telemetry/temperature', {qos: 1});
mqtt.subscribe('plant/+/+/+/alarm/state', {qos: 2});

// Periodic system check
setInterval(() => {
  mqtt.publish('plant/area1/line2/tempcontroller01/command/request', {
    cmd_id: uuid(),
    command: 'get_statistics',
    parameters: { period: 'today' }
  });
}, 300000); // Every 5 minutes
```

### 2. Predictive Maintenance
```javascript
// Monitor sensor health
mqtt.publish('plant/area1/line2/tempcontroller01/command/request', {
  cmd_id: uuid(),
  command: 'get_trend_analysis',
  parameters: {
    points: 'all',
    time_window: '7d',
    analysis_type: ['anomalies']
  }
});
```

### 3. Alarm Integration with SCADA
```javascript
// Forward critical alarms to SCADA
mqtt.on('message', (topic, message) => {
  if (topic.includes('/alarm/state')) {
    const alarm = JSON.parse(message);
    if (alarm.priority === 'CRITICAL') {
      scada.createAlarm({
        source: alarm.device_id,
        point: alarm.point_name,
        value: alarm.temperature,
        priority: 'HIGH'
      });
    }
  }
});
```

### 4. Automated Response System
```javascript
// Auto-acknowledge and control based on conditions
mqtt.on('message', (topic, message) => {
  if (topic.includes('/alarm/state')) {
    const alarm = JSON.parse(message);
    
    // Auto-acknowledge low priority alarms during night shift
    if (alarm.priority === 'LOW' && isNightShift()) {
      mqtt.publish('plant/area1/line2/tempcontroller01/command/request', {
        cmd_id: uuid(),
        command: 'acknowledge_alarm',
        parameters: {
          alarm_id: alarm.alarm_id,
          note: 'Auto-acknowledged during night shift'
        }
      });
    }
    
    // Emergency shutdown for critical temperature
    if (alarm.type === 'HIGH_TEMPERATURE' && 
        alarm.temperature > 100 && 
        alarm.point_name.includes('Reactor')) {
      mqtt.publish('plant/area1/line2/tempcontroller01/command/request', {
        cmd_id: uuid(),
        command: 'control_relay',
        parameters: {
          relay_number: 1,
          action: 'off',
          override_mode: true
        }
      });
    }
  }
});
```

### 5. Data Historian Integration
```javascript
// Periodic data collection for historian
const collectHistoricalData = async () => {
  const response = await mqtt.request({
    command: 'get_historical_data',
    parameters: {
      points: 'all',
      time_range: {
        start: lastCollectionTime,
        end: new Date().toISOString()
      },
      resolution: '1min',
      format: 'json'
    }
  });
  
  historian.ingest({
    source: 'tempcontroller01',
    data: response.data,
    quality: 'GOOD'
  });
};
```

## Conclusion

This MQTT command reference provides comprehensive control over the Temperature Controller system. The command set enables full remote operation, from basic monitoring to complex automation scenarios. When implementing integrations:

1. Start with read-only commands to understand the system
2. Test configuration changes in a safe environment
3. Implement proper error handling and retry logic
4. Monitor system performance and adjust polling rates
5. Use the scheduling system for regular operations
6. Leverage bulk operations for efficiency

For additional support or custom integration requirements, consult the system documentation or contact technical support.