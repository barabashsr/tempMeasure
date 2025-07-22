 **System Status Mode** (Not implemented yet)
   - Enter/Exit: Long button press (>3s)
   - Navigate pages: Short button press
   - Auto-exit: 30s timeout
   - Pages:
     1. Network info (IP, WiFi status)
        Connected to the WiFi:
        ```text
        STATUS: CONNECTED
        IP: 192.168.1.75            # Device IP
        SSID: WiFi_name             # SSID of the wifi access point the device is connected to
        192.168.1.75/dashboard.html # generate from IP
        ```
        Disconnected (AP mode)
        ```text
        STATUS: AP MODE
        IP: 192.168.0.4    # look, what is the default IP
        AP SSID: AP_SSID   # look, what is the default SSID. as i remember, there is no password for the connection, if there is a password - put it in the additional row
        192.168.0.4/cfg    # generate from IP
        ```

        ```text
        STATUS: DISC.
        IP: --.--.--.-- # Device IP
        SSID: --------- # SSID of the wifi access point the device is connected to
        --------------- # generate from IP
        ```

        There could be other cases, investigate and generate in a simmilar way.

     2. System stats (Points configured, Active sensors, Unbound sensors count)
        ```text
        Точки:          27      #count of the points with bound sensors
        DS18B20:        23/25   # count of bound DS18B20 vs.(/) total DS18B20 count
        Pt1000:         4/5     # count of bound Pt1000 vs.(/) total Pt1000 count
        ```
     3. Alarm summary by priority

        ```text
        КРИТ.:      3/8     # count of CRITICAL priority vs.(/) total count of alarms in active or acknowledged stages (--/--  or --/8 if none)
        ВЫС. :      2/8     # count of HIGH priority vs.(/) total count of alarms in active or acknowledged stages (--/--  or --/8 if none)
        СРЕД.:      1/8     # count of MEDIUM priority vs.(/) total count of alarms in active or acknowledged stages (--/--  or --/8 if none)
        НИЗ. :      2/8     # count of LOW priority vs.(/) total count of alarms in active or acknowledged stages (--/--  or --/8 if none)
        ```

     4. Alarm summary by type
        ```text
        ВЫС.T:      3/8     # count of HIGH_TEMP type vs.(/) total count of alarms in active or acknowledged stages (--/--  or --/8 if none)
        НИЗ.T:      2/8     # count of LOW_TEMP type vs.(/) total count of alarms in active or acknowledged stages (--/--  or --/8 if none)
        ОШИБ. :     1/8     # count of SENSOR_ERROR type vs.(/) total count of alarms in active or acknowledged stages 
        ```
     5. Modbus status
        ```text
        STATUS: CONNECTED   #modbus status (Could be disconnected or ERR, or disabled)
        ADDR:   12          # Device address
        PAR:    8N1         #see table for the code
        BR:     9600        #baud rate
        
        ```

        | Shortcut | Data Bits |  Parity  | Stop Bits | Notes                                  |
| :------: | :-------: | :------: | :-------: | :------------------------------------- |
|  **8N1** |     8     | None (N) |     1     | Most common; no parity checking        |
|  **8E1** |     8     | Even (E) |     1     | Adds 1 parity bit (even)               |
|  **8O1** |     8     |  Odd (O) |     1     | Adds 1 parity bit (odd)                |
|  **8N2** |     8     | None (N) |     2     | Two stop bits; no parity               |
|  **7E1** |     7     | Even (E) |     1     | Legacy systems; 7 data + parity + stop |
|  **7O1** |     7     |  Odd (O) |     1     | Legacy systems; 7 data + parity + stop |
|  **7N2** |     7     | None (N) |     2     | Rare; 7 data, 2 stop bits              |
