# Alarm configuration fixing
## Overview
- Each measurement point mast have 3 Alarms from the very startup - high/low temperature, sensor error.
- We turn alarm on and off by `_enable` flag.
- We store alarms in the point `/points2.ini` - including priorities for each type, hysteresis, enable flag
- The temperature controller have ALL the alarms, both enabled and disabled, in `_configuredAlarms` property from the startup.
- We manage all the alarms from the `alarm-config.html` web page.

## allarm-config.html web page
### Table content changes
- Should have `enable` checkboxes - to toggle `_enable` property of alarms of all the types for each measurement point.
- Should allow to edit **Measurement Point name**

## ConfigManager class
- Saves and loads the points config via `/api/alarm-config`
- Loads the alarm config during the startup to temperature controller class

# Plan
## Key changes
- structure of the `/points2.ini` file and related methods to save and load configuration in the ConfigManager class
- `alarm-config.html` - add **checkboxes** and **enable to edit Measurement point name**. 
- **TemperatureController** should keep all the alarms from the very beginning.
- **CSVConfigManager** CSV structure should contain enable fields (checkboxes) for the alarms as well as priorities.
- We should have **only 3 alarm types** -  HIGH_TEMPERATURE, LOW_TEMPERATURE, SENSOR_ERROR,

## Onboarding stage
- **DO NOT** CODE on this stage.
- **DO NOT** follow any other plans you find in the project. We need to implement only this features.
- inspect the code. **THINK HARD** about the alarm logic, class structure. 
- compare the current state of the code and logic with the desired state.
- ASK QUESTIONS
- **THINK HARD** upon the implementation plan.

## Implementation stage
- implement the changes from the plan.
- try to compile the code
- if there are any errors - fix the code. Iterate through the code while it got compiled without any errors.

## Summary
- provide me with the summary of the implemented changes



