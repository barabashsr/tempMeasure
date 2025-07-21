/**
 * @file main.cpp
 * @brief Main application entry point for Industrial Temperature Monitoring System
 * @author Claude Code Session 20250720
 * @date 2025-07-20
 * @details This is the main application file that initializes and coordinates all system components
 *          including temperature sensors, web configuration, Modbus communication, and data logging.
 * 
 * @section dependencies Dependencies
 * - TemperatureController.h for sensor management
 * - ConfigManager.h for web-based configuration
 * - TempModbusServer.h for Modbus RTU communication
 * - IndicatorInterface.h for OLED display and I/O control
 * - TimeManager.h for RTC and NTP time synchronization
 * - LoggerManager.h for data and event logging
 * 
 * @section hardware Hardware Requirements
 * - ESP32-WROVER module
 * - DS18B20 temperature sensors on 4 OneWire buses
 * - PT1000 sensors via MAX31865 on SPI
 * - PCF8575 I/O expander for indicator control
 * - SH1106 OLED display
 * - DS3231 RTC module
 * - SD card for data logging
 * - RS485 interface for Modbus communication
 */

#include <Arduino.h>
#include "TemperatureController.h"
#include "TempModbusServer.h"
#include "ConfigManager.h"
#include "IndicatorInterface.h"
#include <SPI.h>
#include "TimeManager.h"
#include "LoggerManager.h"



// Hardware Pin Definitions

// DS18B20 OneWire Bus Pins
#define BUS1_PIN  4   ///< OneWire bus 1 for DS18B20 sensors
#define BUS2_PIN  5   ///< OneWire bus 2 for DS18B20 sensors
#define BUS3_PIN  18  ///< OneWire bus 3 for DS18B20 sensors
#define BUS4_PIN  19  ///< OneWire bus 4 for DS18B20 sensors

// SPI Pins for PT1000 sensors (MAX31865)
#define SCK_PIN  14   ///< SPI clock pin
#define MISO_PIN  12  ///< SPI MISO pin
#define MOSI_PIN  13  ///< SPI MOSI pin

// PT1000 Chip Select Pins
#define CS1_PIN  32   ///< Chip select for PT1000 sensor 1
#define CS2_PIN  33   ///< Chip select for PT1000 sensor 2
#define CS3_PIN  26   ///< Chip select for PT1000 sensor 3
#define CS4_PIN  27   ///< Chip select for PT1000 sensor 4

#define CS5_PIN_TF_CARD  0  ///< Chip select for SD card

// RS485 Communication Pins
#define RX_PIN  22    ///< RS485 receive pin
#define TX_PIN  23    ///< RS485 transmit pin
#define DE_PIN  -1    ///< RS485 driver enable pin (-1 = not used)

// I2C pins optimized for ESP32-WROVER (avoiding pin conflicts)
#define I2C_SDA 21    ///< I2C data pin (default, available)
#define I2C_SCL 25    ///< I2C clock pin (alternative, GPIO 22 used by RS485)
#define PCF_INT 34    ///< PCF8575 I/O expander interrupt pin

// Global System Components

/// Indicator interface for OLED display and I/O control (I2C address 0x20)
IndicatorInterface indicator(Wire, 0x20, PCF_INT);

/// OneWire bus pin array for DS18B20 sensors
uint8_t onwWirePins[4] = {BUS1_PIN, BUS2_PIN, BUS3_PIN, BUS4_PIN};

/// SPI chip select pin array for PT1000 sensors
uint8_t csPins[4] = {CS1_PIN, CS2_PIN, CS3_PIN, CS4_PIN};

/// Main temperature controller managing all sensors and measurement points
TemperatureController controller(onwWirePins, csPins, indicator);

/// Web-based configuration manager (dynamically allocated)
ConfigManager* configManager;

/// Modbus RTU server for industrial communication (dynamically allocated)
TempModbusServer* modbusServer;

/// Time manager for RTC and NTP synchronization
TimeManager timeManager(I2C_SDA, I2C_SCL);

/// Data and event logger with SD card storage
LoggerManager logger(controller, timeManager, SD);

/**
 * @brief System initialization function
 * @details Initializes all hardware components, communication interfaces,
 *          and system managers in the correct sequence
 */
void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    while (!Serial) {} // Wait for serial port to be ready
    
    // Initialize I2C bus for RTC, OLED, and I/O expander
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000); // 100kHz for reliability

    // Initialize SPI bus for PT1000 sensors and SD card
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);



    // Initialize time manager with RTC
    if (timeManager.init()) {
        Serial.println("TimeManager initialized successfully");
        
        // Set timezone (GMT+3 for Moscow/local time)
        timeManager.setTimezone(3, 0);
    } else {
        Serial.println("TimeManager initialization failed");
    }

    // Configure data logging system
    logger.setLogDirectory("/data");           // Measurement data directory
    logger.setAlarmStateLogDirectory("/alarms"); // Alarm state log directory
    logger.setEventLogDirectory("/events");     // Event log directory
    logger.setLogFrequency(2000);               // Log every 2 seconds
    logger.setDailyFiles(true);                 // Create new file each day
    logger.setEnabled(true);                    // Enable logging

    // Initialize SD card for data logging
    if (!SD.begin(CS5_PIN_TF_CARD)) {
        Serial.println("SD Card initialization failed - logging disabled");
        logger.setEnabled(false);
    } else {
        Serial.println("SD Card initialized successfully");
    }
    
    // Initialize logging system
    if (!logger.init()) {
        Serial.println("Logger initialization failed");
    } else {
        Serial.println("Logger initialized successfully");
    }
    
    // Log system startup event
    logger.info("SYSTEM", "Temperature controller started");




    

    // Configure SPI chip select pins for PT1000 sensors
    pinMode(CS1_PIN, OUTPUT);
    pinMode(CS2_PIN, OUTPUT); 
    pinMode(CS3_PIN, OUTPUT);
    pinMode(CS4_PIN, OUTPUT);
    
    // Set all CS pins HIGH initially (SPI devices inactive)
    digitalWrite(CS1_PIN, HIGH);
    digitalWrite(CS2_PIN, HIGH);
    digitalWrite(CS3_PIN, HIGH);
    digitalWrite(CS4_PIN, HIGH);



    // System identification
    Serial.println("\n=== Industrial Temperature Monitoring System ===");
    Serial.println("Hardware: ESP32-WROVER with multi-sensor support");
    Serial.println("Features: DS18B20, PT1000, Modbus RTU, Web Config");
    Serial.println("===============================================\n");
    
    // Initialize temperature controller
    controller.begin();
    Serial.println("Temperature controller initialized");
    
    // Initialize web-based configuration manager
    configManager = new ConfigManager(controller);
    if (!configManager->begin()) {
        Serial.println("Failed to initialize configuration manager");
    } else {
        Serial.println("Configuration manager initialized");
    }
    
    // Apply configuration settings to controller
    controller.setDeviceId(configManager->getDeviceId());
    controller.setMeasurementPeriod(configManager->getMeasurementPeriod() * 1000); // Convert to milliseconds
    Serial.printf("Device ID: %d, Measurement period: %d ms\n", 
                  configManager->getDeviceId(), 
                  configManager->getMeasurementPeriod() * 1000);
    
    // Discover and initialize temperature sensors
    Serial.println("Discovering sensors...");
    controller.discoverDS18B20Sensors();  // Auto-discover DS18B20 sensors on OneWire buses
    controller.discoverPTSensors();       // Initialize PT1000 sensors on SPI
    Serial.println("Sensor discovery completed");
    // Initialize Modbus RTU server if enabled in configuration
    if (configManager->isModbusEnabled()) {
        Serial.printf("Initializing Modbus RTU server (Address: %d, Baud: %d)...\n",
                     configManager->getModbusAddress(),
                     configManager->getModbusBaudRate());
        
        modbusServer = new TempModbusServer(
            controller.getRegisterMap(),
            configManager->getModbusAddress(),
            Serial2,
            RX_PIN,
            TX_PIN,
            DE_PIN,
            configManager->getModbusBaudRate()
        );
        
        if (modbusServer->begin()) {
            Serial.println("Modbus RTU server started successfully");
        } else {
            Serial.println("Failed to start Modbus RTU server");
        }
    } else {
        Serial.println("Modbus RTU server disabled in configuration");
    }


    // Start time manager services
    if (timeManager.begin()) {
        Serial.println("Time manager services started");
        
        // Sync with NTP if WiFi is already connected
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Syncing time with NTP server...");
            timeManager.setTimeFromNTP();
        }
    }
    
    // Start logging services
    logger.begin();
    
    Serial.println("\n*** SYSTEM INITIALIZATION COMPLETE ***");
    Serial.println("System is now running...");
    Serial.println("Access web interface for configuration\n");
}

/**
 * @brief Main system loop
 * @details Continuously updates all system components including time management,
 *          configuration, temperature monitoring, and data logging
 */
void loop() {
    // Update time manager (handles RTC sync and NTP updates)
    timeManager.update();
    
    // Update configuration manager (handles web server and WiFi portal)
    configManager->update();
    
    // Update temperature controller (reads sensors and updates measurement points)
    controller.update();
    
    // Process any pending Modbus commands
    if (modbusServer) {
        modbusServer->processCommands();
    }
    
    // Periodic status output (when not in configuration portal mode)
    static unsigned long lastPrintTime = 0;
    if (!configManager->isPortalActive() && 
        millis() - lastPrintTime > controller.getMeasurementPeriod()) {
        
        // Uncomment for detailed status output:
        // Serial.println("\nSystem Status:");
        // Serial.println(controller.getSystemStatusJson());
        // Serial.println(controller.getPointsJson());
        
        lastPrintTime = millis();
    }
    
    // Update logger (handles periodic data logging and file management)
    logger.update();
    
    // Brief delay to prevent excessive CPU usage
    delay(100);
}
