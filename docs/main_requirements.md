In addition
- I also need to have **hysteresis implementation in place**
Alarm state transitions works well, so do not change it just extend.
- The **config assist library uses its own config files handler**. Study the code for the implementation
- **Alarm and thresholds configuring via the Modbus RTU should be triggered explicitly** as the master could write zero or undefined values from the startup if not configured properly
- **Alarm configuration page should contain also threshold configuration**, point name configuration etc. It could be done in a dedicated modal, but i do not like the way the alarm configuration modal works now. it should show all the alarms, thresholds and other configurable field at once. 
- **Sensor binding mechanics should remain the same as in the old code** with point address check for different types of sensors. But instead of just inputing a point number there should be a dropdown with point names and autocomplete if it’s possible on esp32.
- I do not like how indication with OLED and LEDs works now. It’s inconsistent and not clear for the user. **Indicator interface logic does not work properly** as well check it carefully. different dislpay and LED blinkings work bad.
- **relay outputs should be able to be controlled via ModbusRTU**.
- **Use the original PCF8575 logic as it’s proven**. Do not change it much, just extend
- When a list of alarms shows the LED indicators should duplicate alarm priority from the screen
- I need a separate document for modbus implementation - register map table, examples, usage patterns etc.
- **For the temperature trend charts use modals to show trends** and **use log file to populate the trend**. It could be called from the dashboard page by clicking on some trend icon in a table row. Place the markers with time stamp and alarm type in a tooltip for alarms rising events on the charts to give the user the most important information
- **The device has only one button, no any additional phisical input controls**. We can use just it.
- **use short button presses to any type of acknowledgement** (alarms, during the startup). **Use long press to enter and exit system status mode**, use short presses for moving thru the pages
- **log system works pretty well at the moment**, just one consideration - too many records in the event log. e.g. creating 20 alarms should be wrapped in one message - 20 alarms created.
- **When on display we work only with mesurement points**. If we have any sensors without any point bound to it, we just show it in the system status page like Unbound sensors 5 pcs. The user works with the measurement points, not with sensors. Sensors are just devices with ROM numbers and we do not show them to the end user. We show the point with bound working (with no errors) sensor with a number and the name
- **Do not mess sensors and measurement points** - those are separate classes and separate abstractions - sensor abstracts the measuring device, Measurement point abstracts a place or a piece of equipment which temperature is measured by the sensor.
- When we have a list of unacknowledged alarms the first is the higher priority alarm and we do not cycle trough the list, we a re waiting while the user press the button. We acknowledge the alarms by short press as mentioned already.
- **Current representation of the alarm on OLED is ok**. We should show the point name, current temp, alarm type, alarm status and other alarm and point related information. **Add timestamp to this screen**
- **Change the way how scrolling on display works**. It should go round - show the beginning of the string after some spaces after the end of the screen but not jump back and place the beginning to the first character 

Provide me with a table for the notification system based on alarm priority and alarm state


