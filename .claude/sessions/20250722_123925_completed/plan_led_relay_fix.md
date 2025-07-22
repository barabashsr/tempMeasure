# LED and Relay Fix Planning Document

## Problem Analysis

### Issue 1: YellowLED Mimics Relay2
- Both YellowLED and Relay2 blink with same timing (2s on/30s off) for medium priority alarms
- This creates visual confusion as they appear synchronized
- Root cause: Lines 546 and 551 in TemperatureController.cpp both use same timing

### Issue 2: Blinking Process Restarts in Cycles
- `updateIndicators()` calls `stopBlinking()` for all ports at the beginning (lines 516-518)
- Then restarts blinking based on alarm states
- This causes interruption and restart of blink cycles every time updateIndicators() is called
- Makes the blinking appear jerky/interrupted instead of smooth

### Issue 3: Port Configuration Already Correct
- The provided port configuration matches current implementation
- No changes needed to port mapping

## Solution Plan

### Fix 1: Only Stop/Start Blinking When State Changes
- Track whether a port should be blinking
- Only call stopBlinking/startBlinking when the blinking state actually changes
- This prevents unnecessary restarts

### Fix 2: Use Different Timing for YellowLED
- Keep Relay2 at 2s on/30s off (for beacon visibility)
- Change YellowLED to faster blink pattern (e.g., 500ms on/500ms off)
- This visually differentiates LED from relay

### Implementation Steps
1. Add state tracking for blinking requirements
2. Modify updateIndicators() to only change blinking when needed
3. Adjust YellowLED timing to differentiate from Relay2
4. Test all alarm priority combinations

## Questions for User
1. What timing would you prefer for YellowLED blinking? (currently same as Relay2: 2s on/30s off)
2. Should the fix maintain backward compatibility with existing alarm behavior?
3. Are there any other LED/relay timing preferences?

## Updated Requirements Based on Documentation

### LED Logic (from briefs):
- **GREEN**: Solid when no alarms (system OK)
- **RED**: Solid for CRITICAL priority alarms
- **YELLOW**: Solid for HIGH priority alarms (NOT blinking)
- **BLUE**: Solid for MEDIUM priority alarms, Blinking for LOW priority alarms

### Current Implementation Issues:
1. YellowLED is blinking for medium priority - should be BlueLED solid
2. YellowLED should be solid for HIGH priority alarms
3. BlueLED should be solid for MEDIUM, blinking for LOW

### Relay Logic (from briefs):
| Priority | Active State | Acknowledged State |
|----------|-------------|-------------------|
| CRITICAL | Siren ON + Beacon ON (constant) | Beacon ON (constant) |
| HIGH | Beacon ON (constant) | Beacon ON (blink 2s on/30s off) |
| MEDIUM | Beacon ON (blink 2s on/30s off) | Beacon OFF |
| LOW | No relay action | No relay action |

### Required Changes:
1. Fix LED assignments:
   - YellowLED: Solid for HIGH priority only
   - BlueLED: Solid for MEDIUM, Blinking for LOW
   - Remove YellowLED blinking completely
2. Fix blinking restart issue by tracking state
3. Ensure relay logic matches the brief