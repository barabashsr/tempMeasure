## Planning Update: 2025-01-20 23:32

### Investigation Results:
- **Alarm Types**: Found 3 types in AlarmType enum: HIGH_TEMPERATURE, LOW_TEMPERATURE, SENSOR_ERROR
- **Current alarm-config.html**:
  - Has priority dropdowns for each alarm type (low/high/error)
  - Allows editing point names (already implemented)
  - Does NOT have enable/disable checkboxes
  - Priorities range from 0 (Disabled) to 4 (Critical)
- **ConfigManager**:
  - Uses `/points2.ini` to store point configurations
  - Currently stores: name, low_alarm, high_alarm thresholds
  - Does NOT store priorities or enable flags in points2.ini
  - API endpoint returns hardcoded priorities (lowPriority: 2, highPriority: 3, errorPriority: 3)
- **TemperatureController**:
  - Has `_configuredAlarms` vector
  - Creates alarms dynamically when conditions are met
  - Does NOT pre-create all alarms at startup
  - Alarms have `_enabled` property but it's not persisted

### Current vs Desired State:
1. **Missing**: Enable checkboxes in alarm-config.html
2. **Missing**: Alarm priorities/enable flags not saved in points2.ini
3. **Missing**: Pre-creation of all 3 alarm types per point at startup
4. **Note**: Point name editing already exists in alarm-config.html

### Implementation Decisions:
1. **Enable/Disable**: Add separate checkboxes alongside priority dropdowns
2. **Storage**: Add fields to points2.ini like `ds_0_low_enable=true`, `ds_0_low_priority=2`
3. **Alarm Creation**: Pre-create all 3 alarms per point at startup:
   - Default: disabled
   - Default priorities: Medium (2) for thresholds, High (3) for sensor error
   - Default thresholds: 0°C low, 80°C high
   - Default hysteresis: 5°C
   - Auto-enable sensor error alarm if sensor is bound
4. **Hysteresis**: Per-point (same for all alarm types)
5. **Name Editing**: Keep existing implementation if working

### Implementation Steps:
- [x] Step 1: Update points2.ini structure to include alarm enable flags and priorities
- [x] Step 2: Modify ConfigManager to save/load alarm settings (enable flags, priorities, hysteresis)
- [x] Step 3: Update TemperatureController to pre-create all alarms at startup
- [x] Step 4: Add enable checkboxes to alarm-config.html
- [x] Step 5: Update /api/alarm-config to return actual alarm settings instead of hardcoded values
- [x] Step 6: Test compilation and fix errors
- [x] Step 7: Test API endpoints functionality

## Planning Update: 2025-01-21 07:07

### Task: Auto-enable sensor error alarm on sensor binding
When a sensor is bound to a measurement point, the sensor error alarm should automatically be enabled.

### Investigation Strategy:
1. Find where sensor binding occurs (likely in sensor-config API)
2. Locate the code that binds sensors to points
3. Add logic to enable sensor error alarm after successful binding

### Expected Files to Modify:
- ConfigManager.cpp - sensor binding API endpoint
- Possibly TemperatureController.cpp - if binding happens there
## Planning Update: 2025-07-21 08:15 - CORRECTED with separate enable bits

### Register Organization Understanding:
The register map follows a different pattern than initially understood:
- **3-digit register format: XYY**
  - X = Section (1-7)
  - YY = Point address (00-59)
  
Example: Register 435
- 4 = Alarm status section
- 35 = Point address 35 (DS18B20)

### Current Register Sections (from README.md):
1. **0-99**: Device information
2. **100-199**: Current temperature (1XX where XX=point)
3. **200-299**: Min temperature (2XX where XX=point)
4. **300-399**: Max temperature (3XX where XX=point)
5. **400-499**: Alarm status (4XX where XX=point)
6. **500-599**: Error status (5XX where XX=point)
7. **600-699**: Low temperature thresholds (6XX where XX=point)
8. **700-799**: High temperature thresholds (7XX where XX=point)

### New Registers Needed for Alarm Control:
Based on requirements, we need:
1. **800-859**: Alarm configuration (priorities and enable states)
   - Format: 8XX where XX=point address
   - Each register contains alarm config for one point
   
2. **860-869**: Relay control
   - 860: Relay 1 control (0=Auto, 1=Force Off, 2=Force On)
   - 861: Relay 2 control
   - 862: Relay 3 control
   - 863-865: Relay current states (read-only)
   
3. **870-889**: Hysteresis configuration
   - Format: 87X where X=0-9 for different alarm types or global
   
4. **899**: Command register
   - Write 0x0001 to apply alarm configuration

### Implementation Steps:

#### Step 1: Update RegisterMap.h
- Add new register constants for sections 8XX
- Add storage arrays for alarm config, relay control, hysteresis
- Update validation methods to accept new ranges

#### Step 2: Update RegisterMap.cpp
- Extend `readHoldingRegister()` to handle:
  - 800-859: Read alarm configuration
  - 860-865: Read relay control/status
  - 870-889: Read hysteresis values
- Extend `writeHoldingRegister()` to handle:
  - 800-859: Write alarm configuration
  - 860-862: Write relay control
  - 870-889: Write hysteresis
  - 899: Execute commands

#### Step 3: Add alarm configuration mapping
- Create methods to convert between:
  - Register bits ↔ AlarmPriority enum
  - Enable/disable states
  - Relay control modes

#### Step 4: Implement command execution
- Add `executeCommand()` method
- Handle command 0x0001: Apply alarm configuration
  - Update TemperatureController alarms
  - Save to configuration files

#### Step 5: Update TempModbusServer
- Ensure callbacks properly handle new registers
- Trigger configuration updates when needed
- Update relay outputs based on control registers

### Alarm Configuration Register Format (8XX) - REVISED:
Better bit allocation with separate enable flags:
- Bit 0: Low Temperature Alarm Enable (0=disabled, 1=enabled)
- Bit 1: High Temperature Alarm Enable (0=disabled, 1=enabled)  
- Bit 2: Sensor Error Alarm Enable (0=disabled, 1=enabled)
- Bits 3-4: Low Temperature Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 5-6: High Temperature Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 7-8: Sensor Error Priority (0=Low, 1=Medium, 2=High, 3=Critical)
- Bits 9-15: Reserved

This gives us:
- Independent enable/disable control for each alarm type
- 2 bits for priority (supports 4 priority levels)
- Clear separation between enable state and priority level
EOF < /dev/null

## Planning Update: 2025-07-21 14:10 - Relay Control Based on Alarm Priority

### Investigation Results:
- **Relevant classes**: TemperatureController, IndicatorInterface, RegisterMap, Alarm
- **Key functions**: 
  - TemperatureController::handleAlarmOutputs() - Main relay control logic
  - IndicatorInterface::writePort() - Hardware control interface
  - RegisterMap::setRelayControl/getRelayControl() - Modbus relay control
- **Dependencies**:
  - PCF8575 I/O expander for relay control
  - Alarm priority system (CRITICAL, HIGH, MEDIUM, LOW)
  - Alarm stages (ACTIVE, ACKNOWLEDGED)

### Current Implementation Status:
1. **Working Features**:
   - Basic relay control via IndicatorInterface (Relay1 = Siren, Relay2 = Beacon)
   - Alarm priority detection and counting
   - LED indicator control based on priority
   - Blinking support for Relay2 and LEDs

2. **Missing Features**:
   - Relay 3 support (Modbus-only control)
   - Modbus relay control modes (Auto/Force Off/Force On)
   - Integration between Modbus commands and relay control

### Implementation Steps:
- [x] Step 1: Review existing relay control implementation in handleAlarmOutputs()
- [ ] Step 2: Add Relay3 support to IndicatorInterface port configuration (NEEDS CLARIFICATION: hardware port assignment)
- [ ] Step 3: Extend handleAlarmOutputs() to include Relay3 control logic
- [ ] Step 4: Implement Modbus relay control override functionality
- [ ] Step 5: Create RelayControlMode enum (AUTO, FORCE_OFF, FORCE_ON)
- [ ] Step 6: Modify handleAlarmOutputs() to respect Modbus control modes
- [ ] Step 7: Implement relay status feedback in registers 11-13 (commanded + actual states)
- [ ] Step 8: Update documentation files (README.md, MODBUS_REGISTER_MAP.md, USER_MANUAL_RU.md)
- [ ] Step 9: Test relay control with different alarm priorities
- [ ] Step 10: Add comprehensive Doxygen documentation
- [ ] Step 11: Update RegisterMap to properly handle relay control registers

### Technical Design:

#### 1. Relay Control Modes
```cpp
enum class RelayControlMode : uint16_t {
    AUTO = 0,        // Automatic control based on alarms
    FORCE_OFF = 1,   // Force relay off regardless of alarms
    FORCE_ON = 2     // Force relay on regardless of alarms
};
```

#### 2. Relay Assignment
- Relay 1: Siren (Critical alarms only)
- Relay 2: Beacon (Critical/High/Medium based on state)
- Relay 3: Modbus-controlled only (no automatic alarm control)

#### 3. Priority Logic (from roadmap):
- **CRITICAL Active**: Siren ON + Beacon ON (constant)
- **CRITICAL Acknowledged**: Beacon ON (constant)
- **HIGH Active**: Beacon ON (constant)
- **HIGH Acknowledged**: Beacon ON (blink 2s on/30s off)
- **MEDIUM Active**: Beacon ON (blink 2s on/30s off)
- **MEDIUM Acknowledged**: Beacon OFF
- **LOW**: No relay action

#### 4. Modbus Integration
- Registers 860-862: Relay control modes (AUTO=0, FORCE_OFF=1, FORCE_ON=2)
- Registers 11-13: Relay status feedback (bit 0: commanded state, bit 1: actual state)
- Command register 899: Apply configuration changes

#### 5. Implementation Notes (from Q&A):
- **Relay 3 Hardware Port**: NEEDS CLARIFICATION - leave placeholder comment in code
- **Modbus Override**: When in FORCE_ON/FORCE_OFF, completely ignore alarm states
- **Persistence**: Control modes NOT saved, default to OFF on startup
- **Relay 3 Purpose**: Spare output, Modbus-only control, no alarm connection
- **Blinking Override**: Modbus commands stop blinking until AUTO mode restored
- **Status Feedback**: Report both commanded and actual states in registers 11-13