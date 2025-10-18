# Doxygen Documentation Setup for Embedded Projects

## Initial Setup

### 1. Install Doxygen
```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows (use installer from)
# https://www.doxygen.nl/download.html
```

### 2. Generate Initial Configuration
```bash
# In project root
doxygen -g Doxyfile
```

### 3. Configure for Embedded C++ Projects

Edit `Doxyfile` with these optimized settings:

```ini
# Project settings
PROJECT_NAME           = "Your Embedded Project"
PROJECT_BRIEF          = "PlatformIO-based embedded system"
OUTPUT_DIRECTORY       = docs/doxygen

# Input settings
INPUT                  = src lib include
FILE_PATTERNS          = *.c *.cpp *.h *.hpp *.ino
RECURSIVE              = YES

# Embedded-specific settings
OPTIMIZE_OUTPUT_FOR_C  = YES
TYPEDEF_HIDES_STRUCT   = YES
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = YES
EXTRACT_STATIC         = YES

# Documentation extraction
JAVADOC_AUTOBRIEF      = YES
QT_AUTOBRIEF           = YES
MULTILINE_CPP_IS_BRIEF = YES

# Source browsing
SOURCE_BROWSER         = YES
INLINE_SOURCES         = NO  # Set YES for full source in docs
REFERENCED_BY_RELATION = YES
REFERENCES_RELATION    = YES

# Diagrams and graphs
HAVE_DOT               = YES
CALL_GRAPH             = YES
CALLER_GRAPH           = YES
CLASS_DIAGRAMS         = YES
COLLABORATION_GRAPH    = YES
UML_LOOK               = YES
DOT_IMAGE_FORMAT       = svg

# HTML output
GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_FILE_EXTENSION    = .html
GENERATE_TREEVIEW      = YES
SEARCHENGINE           = YES

# Disable unused outputs
GENERATE_LATEX         = NO
GENERATE_RTF           = NO
GENERATE_MAN           = NO
GENERATE_XML           = YES  # Keep for parsing

# Hardware-specific tags
ALIASES                = "hardware=\par Hardware Requirements:\n" \
                         "timing=\par Timing Constraints:\n" \
                         "memory=\par Memory Usage:\n" \
                         "power=\par Power Consumption:\n"
```

## Documentation Generation Workflow

### Automated Generation Script

Create `scripts/generate_docs.sh`:

```bash
#!/bin/bash
# Generate documentation with statistics

echo "Generating Doxygen documentation..."

# Clean previous docs
rm -rf docs/doxygen/html docs/doxygen/xml

# Generate docs
doxygen Doxyfile 2>&1 | tee docs/doxygen/doxygen.log

# Count results
echo ""
echo "Documentation Statistics:"
echo "- HTML files: $(find docs/doxygen/html -name "*.html" | wc -l)"
echo "- Classes documented: $(grep -c "class=\"el\"" docs/doxygen/html/annotated.html 2>/dev/null || echo 0)"
echo "- Files documented: $(grep -c "class=\"el\"" docs/doxygen/html/files.html 2>/dev/null || echo 0)"

# Check for warnings
WARNINGS=$(grep -c "warning:" docs/doxygen/doxygen.log)
if [ $WARNINGS -gt 0 ]; then
    echo "- Warnings: $WARNINGS (see docs/doxygen/doxygen.log)"
else
    echo "- Warnings: None"
fi

# Open in browser (optional)
if command -v xdg-open &> /dev/null; then
    xdg-open docs/doxygen/html/index.html
elif command -v open &> /dev/null; then
    open docs/doxygen/html/index.html
fi
```

Make it executable:
```bash
chmod +x scripts/generate_docs.sh
```

### Git Hook for Automatic Documentation

Create `.git/hooks/pre-commit`:

```bash
#!/bin/bash
# Auto-generate docs before commit

# Check if any source files changed
if git diff --cached --name-only | grep -qE '\.(cpp|c|h|hpp)$'; then
    echo "Source files changed, regenerating documentation..."
    doxygen Doxyfile
    git add docs/doxygen/
fi
```

Make it executable:
```bash
chmod +x .git/hooks/pre-commit
```

## Doxygen Comment Examples for Embedded Systems

### File Header
```cpp
/**
 * @file SensorDriver.cpp
 * @brief High-level driver for differential pressure sensor
 * @author Your Name
 * @date 2024-01-20
 * @version 1.0
 * 
 * @details This driver provides abstraction for the BMP280 pressure sensor
 * connected via I2C. It handles initialization, calibration, and continuous
 * sampling with DMA support.
 * 
 * @hardware
 * - BMP280 sensor on I2C1 (SCL: GPIO22, SDA: GPIO21)
 * - Pull-up resistors: 4.7kΩ
 * - Supply voltage: 3.3V
 * 
 * @timing
 * - Sampling rate: 10Hz max
 * - I2C clock: 400kHz
 * - Conversion time: 5.5ms typical
 * 
 * @memory
 * - Stack usage: 256 bytes
 * - Static allocation: 64 bytes
 * - DMA buffer: 128 bytes
 */
```

### Class Documentation
```cpp
/**
 * @class PressureSensor
 * @brief Manages differential pressure measurements
 * 
 * @details This class provides thread-safe access to pressure readings
 * with automatic calibration and temperature compensation.
 * 
 * Example usage:
 * @code
 * PressureSensor sensor(I2C1, 0x76);
 * sensor.begin();
 * float pressure = sensor.readPressure();
 * @endcode
 * 
 * @note ISR-safe methods are marked explicitly
 * @warning Do not call blocking methods from interrupts
 */
class PressureSensor {
```

### Function Documentation
```cpp
/**
 * @brief Initialize the pressure sensor
 * 
 * @param[in] i2c_bus Pointer to I2C peripheral (I2C1 or I2C2)
 * @param[in] address I2C address (0x76 or 0x77)
 * 
 * @return Initialization status
 * @retval true Sensor initialized successfully
 * @retval false Communication error or sensor not found
 * 
 * @pre I2C peripheral must be initialized
 * @post Sensor is in normal mode with default settings
 * 
 * @timing Blocking call, takes up to 50ms
 * @memory Allocates 64 bytes for calibration data
 * 
 * @code
 * if (!sensor.begin(I2C1, 0x76)) {
 *     Serial.println("Sensor init failed!");
 * }
 * @endcode
 * 
 * @see setMode(), setOversampling()
 */
bool begin(I2C_HandleTypeDef* i2c_bus, uint8_t address);
```

### ISR-Safe Function
```cpp
/**
 * @brief Get last pressure reading (ISR-safe)
 * 
 * @return Pressure in Pascals
 * 
 * @note This function is safe to call from interrupts
 * @warning Returns cached value, not fresh reading
 * 
 * @par Implementation:
 * Uses atomic operations to ensure thread safety
 */
float getLastPressure() const;
```

### Hardware Register Documentation
```cpp
/**
 * @def BMP280_REG_CTRL_MEAS
 * @brief Control measurement register
 * 
 * @details
 * Bit 7-5: Temperature oversampling (osrs_t)
 * Bit 4-2: Pressure oversampling (osrs_p)  
 * Bit 1-0: Power mode
 * 
 * @see BMP280 datasheet section 4.3.4
 */
#define BMP280_REG_CTRL_MEAS 0xF4
```

## Parsing Doxygen Output for Claude

### Quick Search Commands
```bash
# Find all classes
grep -h "class=" docs/doxygen/html/annotated.html | sed 's/.*">\(.*\)<\/a>.*/\1/'

# Find all public methods of a class
grep -A10 "Public Member Functions" docs/doxygen/html/class*.html

# Search for specific functionality
grep -r "pressure" docs/doxygen/html/ --include="*.html" | grep -v ".js"

# Find all TODOs
grep -r "@todo\|TODO" docs/doxygen/html/
```

### Python Script for Doc Extraction
```python
#!/usr/bin/env python3
"""extract_docs.py - Extract key info from Doxygen HTML"""

import re
from pathlib import Path
from bs4 import BeautifulSoup

def extract_class_info(html_dir):
    """Extract all classes and their methods"""
    classes = {}
    
    # Parse annotated.html for class list
    with open(f"{html_dir}/annotated.html", 'r') as f:
        soup = BeautifulSoup(f, 'html.parser')
        for link in soup.find_all('a', class_='el'):
            class_name = link.text
            class_file = link['href']
            
            # Parse individual class file
            with open(f"{html_dir}/{class_file}", 'r') as cf:
                class_soup = BeautifulSoup(cf, 'html.parser')
                
                # Extract methods
                methods = []
                for method in class_soup.find_all('td', class_='memname'):
                    method_text = method.get_text(strip=True)
                    if method_text:
                        methods.append(method_text)
                
                classes[class_name] = {
                    'file': class_file,
                    'methods': methods
                }
    
    return classes

# Usage
if __name__ == "__main__":
    classes = extract_class_info("docs/doxygen/html")
    for name, info in classes.items():
        print(f"\nClass: {name}")
        print(f"Methods: {len(info['methods'])}")
        for method in info['methods'][:5]:  # First 5 methods
            print(f"  - {method}")
```

## Integration with Claude Code

### Session Start Script
Create `.claude/scripts/start_session.sh`:

```bash
#!/bin/bash
# Start a new Claude session with fresh docs

# Generate docs
echo "📚 Generating documentation..."
doxygen Doxyfile > /dev/null 2>&1

# Create session
SESSION_ID=$(date +%Y%m%d_%H%M%S)
mkdir -p .claude/sessions/$SESSION_ID
ln -sfn .claude/sessions/$SESSION_ID .claude/sessions/current

# Create summary
cat > .claude/sessions/current/DOC_SUMMARY.md << EOF
# Documentation Summary
Generated: $(date)

## Classes Found
$(grep -c "class=\"el\"" docs/doxygen/html/annotated.html || echo 0) classes documented

## Key Modules
$(ls src/*.cpp 2>/dev/null | head -5)

## Search Index
Ready at: docs/doxygen/html/search/
EOF

echo "✅ Session $SESSION_ID ready!"
echo "📖 Documentation available at: docs/doxygen/html/index.html"
```

### Best Practices

1. **Update Doxyfile for each project** - Adjust INPUT paths and FILE_PATTERNS
2. **Use @todo tags** - Makes finding work items easy
3. **Document hardware connections** - Use custom @hardware tags
4. **Include code examples** - Use @code blocks
5. **Cross-reference** - Use @see tags liberally
6. **Keep comments near code** - Easier to maintain
7. **Generate before each session** - Ensures fresh documentation

This setup enables Claude to work primarily from documentation rather than source files, dramatically reducing token usage while maintaining full project understanding.