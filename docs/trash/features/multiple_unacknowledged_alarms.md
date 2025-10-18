# Multiple Unacknowledged Alarms Feature

## Overview
This feature enhances the temperature monitoring system to properly handle multiple simultaneous alarms with improved display formatting and circular text scrolling.

## Key Enhancements

### 1. Enhanced Alarm Display (3-Line Format)
The alarm display now uses a 3-line format:
- **Line 1**: Priority letter (C/H/M/L) + Alarm type (with circular scrolling for long text)
- **Line 2**: Temperature/Status information (with circular scrolling for long text)
- **Line 3**: Alarm counter (e.g., "1/3") + Current time (HH:MM)

Example display:
```
H: High Temp Alarm
Point1: 35.2°C
1/3  14:25
```

For acknowledged alarms, "ACK" is appended to line 1:
```
H: High Temp Alarm ACK
Point1: 35.2°C
2/5  14:26
```

### 2. Priority Indicators
- **C**: Critical priority
- **H**: High priority
- **M**: Medium priority
- **L**: Low priority

Priority is shown both as a letter prefix on the display and via LED colors.

### 3. Circular Text Scrolling
- Long text on any line automatically scrolls in a circular pattern
- Text seamlessly wraps around with a 3-space separator
- No jarring resets - continuous smooth scrolling
- Applied automatically to all text lines that exceed display width

### 4. Alarm Queue Management
- Maintains existing logic: highest priority alarm shown first
- No automatic rotation - advances only after acknowledgment
- Separate queues for active and acknowledged alarms
- Counter shows position in queue (e.g., "1/3" for first of three alarms)

### 5. Priority-Based LED/Relay Control (Unchanged)
System automatically activates outputs based on highest priority alarm:
- **Critical**: Red LED solid, Siren + Beacon
- **High**: Yellow LED solid, Beacon constant
- **Medium**: Blue LED solid, Beacon blinking
- **Low**: Blue LED blinking, No relay action

## Implementation Details

### Modified Files
1. **TemperatureController.cpp**:
   - Enhanced `_displayNextActiveAlarm()` with alarm counter and priority indicators
   - Enhanced `_displayNextAcknowledgedAlarm()` with ACK prefix
   - Updated `_handleAlarmDisplayRotation()` for automatic cycling
   - Enhanced `_checkButtonPress()` for long press detection
   - Added `_displayAlarmSummary()` and `_getAlarmCounts()`

2. **IndicatorInterface.cpp**:
   - Implemented circular text scrolling in `_drawTextLine()`
   - Updated `_handleScrolling()` for seamless circular behavior

### Usage

#### Normal Operation
- System automatically displays active alarms in priority order
- Each alarm shows for 3 seconds before rotating to next
- Alarm counter shows position in queue (e.g., "2/5")
- Priority indicator shows urgency level

#### Acknowledging Alarms
1. Press button briefly to acknowledge current alarm
2. System automatically advances to next active alarm
3. Acknowledged alarms move to separate queue with "ACK" prefix

#### Viewing Alarm Summary
1. Hold button for 2 seconds
2. Summary shows total active and acknowledged counts
3. Release button to return to normal display

## Testing

A comprehensive test suite is provided in `test/test_multiple_alarms.cpp` with scenarios:
1. Multiple high temperature alarms with different priorities
2. Alarm acknowledgment sequence
3. Mixed alarm types (high/low temp, sensor errors)
4. Alarm summary display

## Benefits
- Operators can quickly see total alarm count
- Priority indicators help focus on critical issues
- Smooth scrolling improves readability
- Acknowledgment tracking prevents missing alarms
- Summary view provides quick system status