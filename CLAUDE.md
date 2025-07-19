# Bash commands
- use git CLI (`gh`) to manage git features
- when use any bash commands run it with --help first  to understand how to use it
## Platformio commands
### Project Management
pio project init --board uno                    # Initialize for Arduino Uno
pio project init --board esp32dev --ide vscode  # Initialize for ESP32 with VSCode
pio project config                              # Show current config

### Build and Run
pio run                                         # Build all environments
pio run -e uno                                  # Build only uno environment
pio run -t clean                                # Clean build files
pio run -t upload                               # Build and upload
pio run -t upload -e esp32dev                   # Upload to specific environment
pio run -t monitor                              # Build, upload, and monitor
pio run --list-targets                          # List available targets

### Device Management
pio device list                                 # List connected devices
pio device monitor                              # Open serial monitor
pio device monitor -b 115200                    # Monitor at 115200 baud
pio device monitor -p /dev/ttyUSB0              # Monitor specific port

### Library Management
pio lib search ArduinoJson                      # Search for libraries
pio lib install ArduinoJson                     # Install library
pio lib install 64                              # Install by ID
pio lib uninstall ArduinoJson                   # Remove library
pio lib list                                    # List installed libraries
pio lib update                                  # Update all libraries

### Package Management
pio pkg install                                 # Install project dependencies
pio pkg list                                    # List installed packages
pio pkg outdated                                # Show outdated packages
pio pkg update                                  # Update packages

### Platform Management
pio platform list                              # List installed platforms
pio platform search esp32                      # Search for platforms
pio platform install espressif32               # Install platform
pio platform uninstall espressif32             # Remove platform

### Testing
pio test                                        # Run all tests
pio test -e uno                                 # Run tests for environment
pio test --verbose                              # Verbose test output

### Remote Development
pio remote device list                          # List remote devices
pio remote run -t upload                       # Remote upload

### Debugging
pio debug                                       # Start debugging session
pio debug --interface=gdb                      # Debug with specific interface

### Account & Access
pio account show                                # Show account info
pio account login                               # Login to PlatformIO account
pio account logout                              # Logout from account

### System Info
pio system info                                 # Show system information
pio system prune                                # Clean system cache
pio upgrade                                     # Upgrade PlatformIO Core



# Code style
- Use doxygen stile comments. The code should be well documented
- OOP aproach is required. Abstract all the phisical devices, data layers and main logic bloks into separate classes.
- use git `claude-branch` branch for work, if it does not exist, create it from the current branch.
- use consistent HTML CSS stiles across all the HTML files

# Workflow
## Onboarding stage
- **DO NOT CODE** during this stage. The purpose of this stage - to study the code base.
- Ask any questions during the stage process you can't clarify from the project files. ALWAYS provide answer options in test-like manner to speed up the process.
- **Check you MCP tools avaliable**. Inform me if something does not work properly and let's fix it together.
- check if `./*/ONBOARDING_RESULTS.md` file already axists. If yes, study it.
- **Explore folder and file structure of the project**
- Explore `~/.platformio` and `./.pio/libdeps` to understand the libraries used in the project better
- Explore the code. **THINK** about the classes, code logic and user interactions with the device
- Use content7 to understand libraries used in the code
- Explore the `./docs` and other project directories for the projects documentation. `docs/main_requirements.md` - the file you should base your decissions when there are any conflict requirements across the documentation.
- Put a summary of the stage in a `./docs/ONBOARDING_RESULTS.md` file (change or create)

## Planning 
- **DO NOT CODE** during this stage. The purpose of this stage - to create a plan of how to change the code to meet the requirements from the briefs.
- check if `./docs/PLANNING_RESULTS.md` file already axists. If yes, study it.
- **THINK HARD** about the plan of the changing in the code to meet requirements from the brief files. The plan shoud contain **step-by-step** implementation with intermediate testing of the code.
- **THINK** about a series of unit tests, what debug output do you need to run the tests and to make the debug process easy in general.
- Ask any questions during the stage process you can't clarify from the project files. ALWAYS provide answer options in test-like manner to speed up the process.
- Create or update a plan and put it into `./docs/PLANNING_RESULTS.md` file.

## Code changing
- **Implement the code chanhes** from the plan file `./docs/PLANNING_RESULTS.md` of the previous step.
- **Step-by-step** implementation required. 
- After improving an old or introducing some new feature or creating new class check the code for consistancy.
- Try to run the build process and check for any compiler errors and warnings
- Iterate thrugh the code to make the build process clean.
- **Use MCP tools** to extend you abbilities.
- Check if any boards are avaliable (plugged) to the computer. You can ask me to connect the device. Ask me for permission to upload the firmware. If yes, upload and monotor the firmware, apply tests. If you do need any inteructions from the physical world, ask me for them, as well as if you need some help to read from the OLED etc. You can ask me for some images etc.
- If for some reasons you can't run any command, **THINK** about how to solve it, create a plan and provide me with this solution.
- Be sure to typecheck when you’re done making a series of code changes
- Prefer running single tests, and not the whole test suite, for performance.
- If the test passed - go to the next step of the implementation plan.

## Documentation and GIT
- Change `README.md` of the project and any files form the `./docs` directory to keep the documentation relevant to changes you've made.
- If the test passed, commit to `claude-branch` git branch.
