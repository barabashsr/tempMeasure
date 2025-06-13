Ok. Now it works excellent. Thank you.
The next step - we need to handle alarms properly. In the device the "loud" indication is with relays 1 and 2 - there is a siren  on the relay 1 and a blinking beacon on relay2, green, yellow, blue and red LEDs are on teh device, so we need to use two kinds of indication separately and there are different scenarios for different alarm priorities as well. Lets solve the loud notification first. I think we should put all the alarms into a sort of stack sorted by priority and a stage within each priority. If there are any alarms within the group, we handle the loud notification this way (for example):
1. Critical unacknowledged - both siren and beacon are ON constantly
2. High priority unacknowledged, or Critical not cleared but acknowledged - beacon is ON constantly
3. All Critical alarms are resolved, but High priority not cleared but acknowledged, or Medium priority is either active or acknowleged - beacon is getting on from time to time (for 10 secconds once in a few minutes - this also should be a setting)

So actually we need a scenario for each alarm priority with standard options
ACTIVE - siren action, beacon action
ACKNOWLEDGED - siren action, beacon action


Also we should display and handle acknolagement of uresolved alarms in a different way. We should display the highest priority alarm in ACTIVE status. After acknowlegment if there are any unacknowleged alarms we should show next unacknowledged alarm etc. When all the alarms in active status got acknowledged we need to show them in a cycle with some delay (10 secconds - it should be a setting). We should also turn on one of the LEDs - Red for CRITICAL, Yellow for HIGH and Blue for MEDIUM priority.

So, all the alarms should be sorted by priority and status and the loud notification should act according the groups which are not empty. We should start to display alerms in a cycle when all the alarms got acknowledged. We should have settings for the scenario of each alarm priority. We should be able to move acknowledged alarm to active by timer.

This all should be done by TemperatureController class or a new class if it is advisable.
