/**
 * @file IndicatorInterface.h
 * @brief Hardware interface for PCF8575 I/O expander and SH1106 OLED display
 * @author Claude Code Session 20250720
 * @date 2025-07-20
 * @details This file provides a comprehensive interface for controlling PCF8575 I/O expander
 *          and SH1106 OLED display, including port control, interrupt handling, and display management.
 * 
 * @section dependencies Dependencies
 * - PCF8575.h for I/O expander control
 * - U8g2lib.h for OLED display control
 * - Wire.h for I2C communication
 * 
 * @section hardware Hardware Requirements
 * - ESP32 with I2C capability
 * - PCF8575 16-bit I/O expander
 * - SH1106 128x64 OLED display
 */

#ifndef INDICATOR_INTERFACE_H
#define INDICATOR_INTERFACE_H

#include <Arduino.h>
#include <Wire.h>
#include <map>
#include <string>
#include "PCF8575.h"
#include <U8g2lib.h>
#include <vector>
#include "LoggerManager.h"

// Scrolling configuration
#define SCROLL_SPEED_PIXELS 4        // Pixels to scroll per update (default was 2)
#define SCROLL_UPDATE_DELAY_MS 50    // Milliseconds between scroll updates

/**
 * @brief Interface class for PCF8575 I/O expander and OLED display
 * @details Provides comprehensive control over 16-bit I/O expander with named ports,
 *          interrupt handling, blinking control, and OLED display management
 */
class IndicatorInterface {
public:
    /**
     * @brief Structure for managing blinking port states
     * @details Tracks timing and state information for ports that blink automatically
     */
    struct BlinkingPort {
        std::string portName;        ///< Name of the blinking port
        unsigned long onTime;        ///< Duration port stays on (milliseconds)
        unsigned long offTime;       ///< Duration port stays off (milliseconds)
        unsigned long lastToggleTime; ///< Last time port state was toggled
        bool currentState;           ///< Current on/off state
        bool isActive;              ///< Whether blinking is currently active
    };
    /**
     * @brief Constructor for IndicatorInterface
     * @param[in] i2cBus Reference to I2C bus (Wire)
     * @param[in] pcf_i2cAddress I2C address of PCF8575 expander
     * @param[in] intPin GPIO pin for interrupt handling (-1 to disable)
     * @details Initializes I/O expander and OLED display interfaces
     */
    IndicatorInterface(TwoWire& i2cBus, uint8_t pcf_i2cAddress, int intPin = -1);
    
    /**
     * @brief Destructor for IndicatorInterface
     * @details Cleans up resources and disables interrupts
     */
    ~IndicatorInterface();
    
    /**
     * @brief Initialize the indicator interface
     * @return bool True if initialization successful
     * @details Sets up PCF8575, OLED display, and interrupt handling
     */
    bool begin();
    
    /**
     * @brief Update interface state (call in main loop)
     * @details Handles port state updates, blinking, OLED updates, and sleep management
     */
    void update();
    
    // Configuration setters
    /**
     * @brief Set port directions for all 16 ports
     * @param[in] directionMask Bit mask where 0=input, 1=output
     * @details Configures which ports are inputs vs outputs
     */
    void setDirection(uint16_t directionMask);
    
    /**
     * @brief Set port inversion mode for all 16 ports
     * @param[in] modeMask Bit mask where 0=normal, 1=inverted
     * @details Configures logic inversion for each port
     */
    void setMode(uint16_t modeMask);
    
    /**
     * @brief Set names for multiple ports
     * @param[in] portNames Map of port names to port numbers
     * @details Assigns human-readable names to port numbers for easier access
     */
    void setPortNames(const std::map<std::string, uint8_t>& portNames);
    
    /**
     * @brief Set name for a single port
     * @param[in] name Human-readable port name
     * @param[in] portNumber Port number (0-15)
     * @details Assigns a name to a specific port number
     */
    void setPortName(const std::string& name, uint8_t portNumber);
    
    // Port control methods
    /**
     * @brief Write state to a named port
     * @param[in] portName Name of the port to control
     * @param[in] state Desired state (true=high, false=low)
     * @return bool True if port exists and write successful
     * @details Controls output port state using human-readable name
     */
    bool writePort(const std::string& portName, bool state);
    
    /**
     * @brief Write state to a numbered port
     * @param[in] portNumber Port number (0-15)
     * @param[in] state Desired state (true=high, false=low)
     * @return bool True if write successful
     * @details Controls output port state using port number
     */
    bool writePort(uint8_t portNumber, bool state);
    
    /**
     * @brief Write states to multiple ports simultaneously
     * @param[in] portMask 16-bit mask representing desired port states
     * @details Updates all output ports in a single I2C transaction
     */
    void writePorts(uint16_t portMask);
    
    /**
     * @brief Set all output ports to specified state
     * @param[in] state Desired state for all outputs
     * @details Controls all output ports simultaneously
     */
    void setAllOutputs(bool state);
    
    /**
     * @brief Set all output ports to high state
     * @details Convenience method to set all outputs high
     */
    void setAllOutputsHigh();
    
    /**
     * @brief Set all output ports to low state
     * @details Convenience method to set all outputs low
     */
    void setAllOutputsLow();
    
    /**
     * @brief Set inversion mode for a named port
     * @param[in] portName Name of the port
     * @param[in] inverted True to invert logic, false for normal
     * @details Controls whether port logic is inverted
     */
    void setPortInverted(const std::string& portName, bool inverted);
    
    /**
     * @brief Set inversion mode for a numbered port
     * @param[in] portNumber Port number (0-15)
     * @param[in] inverted True to invert logic, false for normal
     * @details Controls whether port logic is inverted
     */
    void setPortInverted(uint8_t portNumber, bool inverted);

    
    // Port reading methods
    /**
     * @brief Get current state of all 16 ports
     * @return uint16_t 16-bit value representing all port states
     * @details Returns raw port states as read from PCF8575
     */
    uint16_t getCurrentState();
    
    /**
     * @brief Read state of a named port
     * @param[in] portName Name of the port to read
     * @return bool Current port state (true=high, false=low)
     * @details Reads input port state using human-readable name
     */
    bool readPort(const std::string& portName);
    
    /**
     * @brief Read state of a numbered port
     * @param[in] portNumber Port number (0-15)
     * @return bool Current port state (true=high, false=low)
     * @details Reads input port state using port number
     */
    bool readPort(uint8_t portNumber);
    
    // Utility methods
    /**
     * @brief Check if port is configured as output
     * @param[in] portNumber Port number (0-15)
     * @return bool True if port is configured as output
     */
    bool isOutput(uint8_t portNumber);
    
    /**
     * @brief Check if port is configured as input
     * @param[in] portNumber Port number (0-15)
     * @return bool True if port is configured as input
     */
    bool isInput(uint8_t portNumber);
    
    /**
     * @brief Check if port has inverted logic
     * @param[in] portNumber Port number (0-15)
     * @return bool True if port logic is inverted
     */
    bool isInverted(uint8_t portNumber);
    
    /**
     * @brief Get port number from port name
     * @param[in] portName Name of the port
     * @return uint8_t Port number (0-15), 255 if name not found
     */
    uint8_t getPortNumber(const std::string& portName);
    
    /**
     * @brief Get port name from port number
     * @param[in] portNumber Port number (0-15)
     * @return std::string Port name, empty string if no name assigned
     */
    std::string getPortName(uint8_t portNumber);
    
    // Interrupt handling
    /**
     * @brief Handle interrupt from PCF8575
     * @details Called when interrupt pin is triggered, reads and processes port changes
     */
    void handleInterrupt();
    
    /**
     * @brief Set callback function for port change interrupts
     * @param[in] callback Function to call when ports change
     * @details Callback receives current state and changed pins as parameters
     */
    void setInterruptCallback(void (*callback)(uint16_t currentState, uint16_t changedPins));
    
    // Debug methods
    /**
     * @brief Print current state of all ports to Serial
     * @details Useful for debugging port configurations and states
     */
    void printPortStates();
    
    /**
     * @brief Print complete port configuration to Serial
     * @details Shows direction, inversion, and name mappings for all ports
     */
    void printConfiguration();

    // OLED control methods
    /**
     * @brief Set OLED sleep delay
     * @param[in] sleepDelay Milliseconds before sleep (-1 = never sleep)
     * @details Controls automatic OLED power saving
     */
    void setOledSleepDelay(long sleepDelay);
    
    /**
     * @brief Set OLED display mode
     * @param[in] lines Number of text lines to display (1-5)
     * @details Configures display layout for text output
     */
    void setOledMode(int lines);
    
    /**
     * @brief Set OLED display mode with small font option
     * @param[in] lines Number of text lines to display (1-5)
     * @param[in] useSmallFont Use smallest available font
     * @details Configures display layout with optional small font
     */
    void setOledModeSmall(int lines, bool useSmallFont = true);
    
    /**
     * @brief Display text buffer on OLED
     * @param[in] buffer Array of strings to display
     * @param[in] bufferSize Number of strings in buffer
     * @details Updates OLED display with provided text
     */
    void printText(String buffer[], int bufferSize);
    
    /**
     * @brief Set OLED blinking mode
     * @param[in] timeOn Milliseconds display is on
     * @param[in] timeOff Milliseconds display is off
     * @param[in] blinkOn True to enable blinking
     * @details Controls OLED blinking for attention-getting displays
     */
    void setOLEDblink(int timeOn, int timeOff, bool blinkOn = true);
    
    /**
     * @brief Turn OLED display off
     * @details Powers down OLED display
     */
    void setOLEDOff();
    
    /**
     * @brief Turn OLED display on
     * @details Powers up OLED display
     */
    void setOLEDOn();
    
    /**
     * @brief Update OLED display (call in main loop)
     * @details Handles scrolling, blinking, and sleep management
     */
    void updateOLED();

    // Special display methods
    /**
     * @brief Push new line to display, shifting others down
     * @param[in] newLine New text line to add at bottom
     * @details Adds new line and scrolls existing lines up
     */
    void pushLine(String newLine);
    
    /**
     * @brief Display large "OK" symbol
     * @details Shows confirmation symbol filling the display
     */
    void displayOK();
    
    /**
     * @brief Display large cross/error symbol
     * @details Shows error symbol in circle filling the display
     */
    void displayCross();
    
    /**
     * @brief Blink between OK symbol and previous text
     * @param[in] blinkDelay Milliseconds between blink states
     * @details Alternates between OK symbol and saved text
     */
    void blinkOK(int blinkDelay);
    
    /**
     * @brief Blink between cross symbol and previous text
     * @param[in] blinkDelay Milliseconds between blink states
     * @details Alternates between cross symbol and saved text
     */
    void blinkCross(int blinkDelay);
    
    /**
     * @brief Stop all blinking and restore normal text
     * @details Ends blinking mode and returns to previous display
     */
    void stopBlinking();

    /**
     * @brief Start blinking a named port
     * @param[in] portName Name of port to blink
     * @param[in] onTime Milliseconds port stays on
     * @param[in] offTime Milliseconds port stays off
     * @details Begins automatic blinking of specified port
     */
    void startBlinking(const std::string& portName, unsigned long onTime, unsigned long offTime);
    
    /**
     * @brief Stop blinking a named port
     * @param[in] portName Name of port to stop blinking
     * @details Ends automatic blinking and leaves port in current state
     */
    void stopBlinking(const std::string& portName);
    
    /**
     * @brief Update all blinking ports (call in main loop)
     * @details Handles timing and state changes for all blinking ports
     */
    void updateBlinking();
    
    /**
     * @brief Check if a port is currently blinking
     * @param[in] portName Name of port to check
     * @return bool True if port is actively blinking
     */
    bool isBlinking(const std::string& portName);


private:
    std::vector<BlinkingPort> _blinkingPorts;  ///< Vector of ports configured for blinking

    // Hardware configuration
    TwoWire* _i2cBus;           ///< Pointer to I2C bus interface
    uint8_t _pcf_i2cAddress;    ///< I2C address of PCF8575 expander
    uint8_t oled_i2cAddress;    ///< I2C address of OLED display
    int _intPin;                ///< GPIO pin for interrupt handling
    PCF8575 _pcf8575;           ///< PCF8575 expander instance
    
    // Port configuration
    uint16_t _directionMask;    ///< Port direction mask (0=input, 1=output)
    uint16_t _modeMask;         ///< Port inversion mask (0=normal, 1=inverted)
    std::map<std::string, uint8_t> _portNames;   ///< Map of port names to numbers
    std::map<uint8_t, std::string> _portNumbers; ///< Map of port numbers to names
    
    // State tracking
    uint16_t _currentState;      ///< Current state of all 16 ports
    uint16_t _lastState;         ///< Previous state for change detection
    unsigned long _lastReadTime; ///< Last time ports were read
    unsigned long _pollInterval; ///< Polling interval for port updates
    
    // Interrupt handling
    volatile bool _interruptFlag; ///< Flag set by interrupt handler
    bool _useInterrupts;         ///< Whether interrupts are enabled
    void (*_interruptCallback)(uint16_t currentState, uint16_t changedPins); ///< User callback function
    
    // Internal methods
    /**
     * @brief Update internal state from PCF8575
     * @details Reads current port states and detects changes
     */
    void _updateState();
    
    /**
     * @brief Clear interrupt flag on PCF8575
     * @details Resets interrupt condition by reading ports
     */
    void _clearInterrupt();
    
    /**
     * @brief Read raw state from PCF8575
     * @return uint16_t Raw 16-bit port state
     */
    uint16_t _readPCF();
    
    /**
     * @brief Write raw state to PCF8575
     * @param[in] state 16-bit state to write
     */
    void _writePCF(uint16_t state);
    
    /**
     * @brief Apply inversion logic to port state
     * @param[in] portNumber Port number (0-15)
     * @param[in] state Logical state
     * @return bool Physical state after inversion logic
     */
    bool _applyModeLogic(uint8_t portNumber, bool state);
    
    /**
     * @brief Reverse inversion logic from physical to logical
     * @param[in] portNumber Port number (0-15)
     * @param[in] state Physical state
     * @return bool Logical state after reversing inversion
     */
    bool _reverseModeLogic(uint8_t portNumber, bool state);
    
    /**
     * @brief Configure interrupt pin for PCF8575
     * @details Sets up GPIO pin for interrupt handling
     */
    void _configureInterruptPin();
    
    // Static interrupt handler
    static IndicatorInterface* _instance;  ///< Static instance for interrupt callback
    
    /**
     * @brief Static interrupt service routine
     * @details IRAM-resident ISR for handling PCF8575 interrupts
     */
    static void IRAM_ATTR _staticInterruptHandler();
    
    static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;  ///< Static OLED display instance

    // OLED configuration
    long _oledSleepDelay;        ///< Sleep delay in milliseconds (-1 = never sleep)
    int _oledLines;              ///< Number of display lines (1-5)
    String _textBuffer[5];       ///< Text buffer for display (max 5 lines)
    int _textBufferSize;         ///< Current number of lines in buffer
    bool _oledOn;                ///< OLED power state
    bool _oledBlink;             ///< Whether OLED is blinking
    int _blinkTimeOn;            ///< Milliseconds OLED stays on during blink
    int _blinkTimeOff;           ///< Milliseconds OLED stays off during blink
    unsigned long _lastBlinkTime; ///< Last blink toggle time
    bool _blinkState;            ///< Current blink state (on/off)
    unsigned long _lastActivityTime; ///< Last user activity time
    bool _oledSleeping;          ///< Whether OLED is currently sleeping
    
    // Scrolling variables
    int _scrollOffset[5];        ///< Horizontal scroll offset for each line
    unsigned long _lastScrollTime; ///< Last scroll update time
    int _scrollDelay;            ///< Delay between scroll steps
    int _charWidth;              ///< Width of characters in pixels
    int _lineHeight;             ///< Height of lines in pixels
    int _maxCharsPerLine;        ///< Maximum characters per display line
    
    // Internal OLED methods
    /**
     * @brief Initialize OLED display
     * @details Sets up SH1106 display and calculates display parameters
     */
    void _initOLED();
    
    /**
     * @brief Update OLED display content
     * @details Redraws display with current text buffer
     */
    void _updateOLEDDisplay();
    
    /**
     * @brief Handle OLED sleep management
     * @details Manages automatic sleep based on activity timeout
     */
    void _handleOLEDSleep();
    
    /**
     * @brief Handle OLED blinking
     * @details Manages display on/off timing for blink mode
     */
    void _handleOLEDBlink();
    
    /**
     * @brief Handle text scrolling
     * @details Updates horizontal scroll position for long text lines
     */
    void _handleScrolling();
    
    /**
     * @brief Draw single text line at specified position
     * @param[in] lineIndex Index of line in text buffer
     * @param[in] yPos Vertical position in pixels
     */
    void _drawTextLine(int lineIndex, int yPos);
    
    /**
     * @brief Calculate display parameters
     * @details Computes character dimensions and layout parameters
     */
    void _calculateDisplayParams();
    
    /**
     * @brief Wake OLED from sleep
     * @details Powers up display and resets activity timer
     */
    void _wakeOLED();
    //void _fixSH1106Offset();
    // Special display state
    String _savedTextBuffer[5];   ///< Backup of text before OK/Cross display
    int _savedTextBufferSize;     ///< Size of saved text buffer
    int _savedOledLines;          ///< Saved number of display lines
    bool _isBlinkingOK;           ///< Whether currently blinking OK symbol
    bool _isBlinkingCross;        ///< Whether currently blinking cross symbol
    int _blinkDelayTime;          ///< Delay time for special symbol blinking
    unsigned long _lastBlinkToggle; ///< Last toggle time for special blinking
    bool _blinkShowSpecial;       ///< Current blink state (true=symbol, false=text)

    // Internal special display methods
    /**
     * @brief Save current text before displaying special symbols
     * @details Backs up current display state for later restoration
     */
    void _saveCurrentText();
    
    /**
     * @brief Restore previously saved text
     * @details Restores display state from before special symbol display
     */
    void _restoreCurrentText();
    
    /**
     * @brief Handle special symbol blinking
     * @details Manages alternation between special symbols and saved text
     */
    void _handleSpecialBlink();

};

#endif
