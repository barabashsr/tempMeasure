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