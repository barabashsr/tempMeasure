# ESP-IDF Migration Plan
## Arduino to ESP-IDF Framework Migration Strategy

**Document Version**: 1.0
**Date**: 2024-10-20
**Project**: Temperature Controller System
**Estimated Duration**: 4 weeks
**Risk Level**: Moderate

---

## Executive Summary

This document outlines a systematic approach to migrate the Temperature Controller System from Arduino framework to pure ESP-IDF framework using PlatformIO's mixed framework support. The migration will be performed class-by-class to minimize risk and maintain system functionality throughout the transition.

### Key Benefits of Migration
- **Performance**: ~20-30% improvement in execution speed
- **Memory**: ~15-20KB RAM savings from Arduino overhead removal
- **Control**: Direct hardware access and configuration
- **Maintenance**: Access to latest ESP-IDF features and security updates
- **Professional**: Industry-standard framework for commercial deployment

### Migration Philosophy
- **Incremental**: One component at a time
- **Testable**: Each phase independently verifiable
- **Reversible**: Can rollback individual components
- **Functional**: System remains operational throughout

---

## Prerequisites

### Development Environment Setup

1. **Update PlatformIO**
```bash
pio upgrade
pio pkg update
```

2. **Install ESP-IDF Components Manager**
```bash
pip install idf-component-manager
```

3. **Backup Current Project**
```bash
git checkout -b espidf-migration
git add -A && git commit -m "Pre-migration backup"
```

### Initial Configuration Change

Modify `platformio.ini` to enable mixed framework:

```ini
[env:esp-wrover-kit]
platform = espressif32
board = esp-wrover-kit
framework = arduino, espidf  # Enable mixed mode!
monitor_speed = 115200
build_flags =
    -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue
    -DCA_USE_LITTLEFS
    -DLOGGER_LOG_LEVEL=3
    -DCA_USE_WIFISCAN=1
    -DCA_USE_TESTWIFI=1
    -DUSE_MIXED_MODE=1  # Add flag for conditional compilation
board_build.partitions = huge_app.csv
board_build.filesystem = littlefs
lib_ldf_mode = chain+

# Existing libraries remain for now
lib_deps =
    # ... existing dependencies ...
```

---

## Library Replacement Matrix

### Verified ESP-IDF Components

| Priority | Arduino Library | ESP-IDF Replacement | Component ID | Status |
|----------|----------------|---------------------|--------------|---------|
| **P1** | ModbusClient | esp-modbus | `espressif/esp-modbus^1.0.12` | ✅ Official |
| **P1** | 256dpi/MQTT | esp-mqtt | Built-in | ✅ Native |
| **P1** | NTPClient | SNTP | Built-in | ✅ Native |
| **P2** | OneWire/Dallas | esp32-owb + esp32-ds18b20 | GitHub | ✅ Mature |
| **P2** | LittleFS | esp_littlefs | `joltwallet/littlefs^1.14.0` | ✅ Official |
| **P2** | PCF8575 | pcf857x | `espressif/pcf857x^1.0.0` | ✅ Official |
| **P3** | RTClib | ds3231 | `espressif/ds3231^1.0.0` | ✅ Official |
| **P3** | U8g2 | u8g2-esp-idf | GitHub/Custom | ⚠️ Port needed |
| **P4** | ConfigAssist | Custom NVS+HTTP | N/A | ❌ Rewrite |
| **P4** | CSV Parser | Custom C++ | N/A | ❌ Rewrite |

### Component Installation Commands

```bash
# Official ESP Registry Components
idf.py add-dependency "espressif/esp-modbus^1.0.12"
idf.py add-dependency "joltwallet/littlefs^1.14.0"
idf.py add-dependency "espressif/pcf857x^1.0.0"
idf.py add-dependency "espressif/ds3231^1.0.0"

# GitHub Components (add to CMakeLists.txt)
# esp32-owb: https://github.com/DavidAntliff/esp32-owb
# esp32-ds18b20: https://github.com/DavidAntliff/esp32-ds18b20
```

---

## Phase 1: Foundation Layer (Week 1)
**Goal**: Establish ESP-IDF build system and migrate utility classes

### Day 1-2: Build System & Logging

#### 1.1 Create ESP-IDF Component Structure
```bash
mkdir -p components/temp_controller
mkdir -p components/temp_controller/include
mkdir -p components/temp_controller/src
```

#### 1.2 Create CMakeLists.txt
```cmake
# components/temp_controller/CMakeLists.txt
idf_component_register(
    SRCS "src/LoggerManager_idf.cpp"
          "src/TimeManager_idf.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_timer driver
    PRIV_REQUIRES esp_wifi lwip
)
```

#### 1.3 Migrate LoggerManager

**Original (Arduino)**:
```cpp
// LoggerManager.cpp
Serial.printf("Temperature: %.2f\n", temp);
File logFile = SD.open(filename, FILE_WRITE);
```

**ESP-IDF Version**:
```cpp
// LoggerManager_idf.cpp
#include "esp_log.h"
#include "esp_littlefs.h"

static const char* TAG = "LoggerManager";

void LoggerManager::logMessage(float temp) {
    ESP_LOGI(TAG, "Temperature: %.2f", temp);

    FILE* f = fopen("/littlefs/log.txt", "a");
    if (f) {
        fprintf(f, "%.2f\n", temp);
        fclose(f);
    }
}
```

#### 1.4 Migrate TimeManager

**Original (Arduino)**:
```cpp
// TimeManager.cpp
#include <NTPClient.h>
NTPClient timeClient(ntpUDP, "pool.ntp.org");
```

**ESP-IDF Version**:
```cpp
// TimeManager_idf.cpp
#include "esp_sntp.h"

void TimeManager::initNTP() {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized");
}
```

### Day 3-4: File System Migration

#### 1.5 Initialize LittleFS for ESP-IDF
```cpp
// filesystem_init.cpp
#include "esp_littlefs.h"

esp_err_t init_filesystem() {
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS");
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}
```

### Day 5: Testing Foundation
- [ ] Verify logging to console works
- [ ] Verify file operations work
- [ ] Verify NTP synchronization
- [ ] Run 24-hour stability test

---

## Phase 2: Hardware Abstraction (Week 1-2)
**Goal**: Migrate sensor interfaces and I/O components

### Day 6-7: Modbus Migration

#### 2.1 Replace ModbusClient with esp-modbus

**Original (Arduino)**:
```cpp
// TempModbusServer.cpp
#include <ModbusRTU.h>
ModbusRTU mb;
mb.begin(&Serial2);
mb.addHreg(HREG_BASE, 0, HREG_COUNT);
```

**ESP-IDF Version**:
```cpp
// TempModbusServer_idf.cpp
#include "mbcontroller.h"

void ModbusServer::init() {
    mb_communication_info_t comm_info = {
        .mode = MB_MODE_RTU,
        .slave_addr = 1,
        .port = UART_NUM_2,
        .baudrate = 9600,
        .parity = MB_PARITY_NONE
    };

    void* mbc_slave_handler = NULL;
    ESP_ERROR_CHECK(mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler));

    mb_register_area_descriptor_t reg_area = {
        .type = MB_PARAM_HOLDING,
        .start_offset = 0,
        .address = (void*)&holding_reg_params,
        .size = sizeof(holding_reg_params)
    };
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));
    ESP_ERROR_CHECK(mbc_slave_start());
}
```

### Day 8-10: OneWire Sensor Migration

#### 2.2 Install esp32-owb Components
```bash
# Add as git submodule
git submodule add https://github.com/DavidAntliff/esp32-owb.git components/esp32-owb
git submodule add https://github.com/DavidAntliff/esp32-ds18b20.git components/esp32-ds18b20
```

#### 2.3 Migrate Sensor Class

**Original (Arduino)**:
```cpp
// Sensor.cpp
#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(pin);
DallasTemperature sensors(&oneWire);
```

**ESP-IDF Version**:
```cpp
// Sensor_idf.cpp
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

class DS18B20Sensor {
    owb_rmt_driver_info rmt_driver_info;
    OneWireBus *owb;
    DS18B20_Info *ds18b20_info;

public:
    void init(gpio_num_t pin) {
        owb = owb_rmt_initialize(&rmt_driver_info, pin,
                                 RMT_CHANNEL_1, RMT_CHANNEL_0);
        owb_use_crc(owb, true);

        // Find devices
        OneWireBus_SearchState search_state;
        owb_search_first(owb, &search_state, &found_device);

        // Initialize DS18B20
        ds18b20_info = ds18b20_malloc();
        ds18b20_init_solo(ds18b20_info, owb);
        ds18b20_use_crc(ds18b20_info, true);
        ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION_12_BIT);
    }

    float readTemperature() {
        ds18b20_convert_all(owb);
        ds18b20_wait_for_conversion(ds18b20_info);

        float temp = 0;
        DS18B20_ERROR error = ds18b20_read_temp(ds18b20_info, &temp);
        if (error != DS18B20_OK) {
            ESP_LOGE(TAG, "Temperature read failed");
            return -127.0;
        }
        return temp;
    }
};
```

### Day 11-12: I/O Expander Migration

#### 2.4 Migrate PCF8575
```cpp
// IndicatorInterface_idf.cpp
#include "pcf857x.h"

class IndicatorInterface {
    i2c_dev_t pcf8575_dev;

    void init() {
        // Initialize I2C
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = GPIO_NUM_21,
            .scl_io_num = GPIO_NUM_22,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = 100000,
        };
        i2c_param_config(I2C_NUM_0, &conf);
        i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

        // Initialize PCF8575
        pcf857x_init_desc(&pcf8575_dev, 0x20, I2C_NUM_0,
                          GPIO_NUM_21, GPIO_NUM_22);
    }

    void setPin(uint8_t pin, bool state) {
        pcf857x_set(&pcf8575_dev, pin, state);
    }
};
```

---

## Phase 3: Communication Layer (Week 2)
**Goal**: Migrate MQTT and prepare web server infrastructure

### Day 13-14: MQTT Migration

#### 3.1 Native ESP-IDF MQTT Implementation

**Original (Arduino)**:
```cpp
// MQTTManager.cpp
#include <MQTT.h>
MQTTClient mqttClient(4096);
mqttClient.begin(broker, port, wifiClient);
mqttClient.publish(topic, payload);
```

**ESP-IDF Version**:
```cpp
// MQTTManager_idf.cpp
#include "mqtt_client.h"

class MQTTManager {
    esp_mqtt_client_handle_t client;

    static void mqtt_event_handler(void *handler_args,
                                  esp_event_base_t base,
                                  int32_t event_id,
                                  void *event_data) {
        esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

        switch (event->event_id) {
            case MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT Connected");
                esp_mqtt_client_subscribe(client, "cmd/+", 0);
                break;
            case MQTT_EVENT_DATA:
                processMessage(event->topic, event->data, event->data_len);
                break;
            case MQTT_EVENT_DISCONNECTED:
                ESP_LOGW(TAG, "MQTT Disconnected");
                break;
        }
    }

public:
    void init(const char* broker_url) {
        esp_mqtt_client_config_t mqtt_cfg = {
            .broker = {
                .address.uri = broker_url,
                .verification = {
                    .use_global_ca_store = true,
                }
            },
            .credentials = {
                .username = config.username,
                .authentication = {
                    .password = config.password,
                }
            },
            .session = {
                .last_will = {
                    .topic = "status",
                    .msg = "offline",
                    .qos = 1,
                    .retain = true,
                }
            }
        };

        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                       mqtt_event_handler, client);
        esp_mqtt_client_start(client);
    }

    void publish(const char* topic, const char* data) {
        esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
    }
};
```

### Day 15-16: RTC Migration

#### 3.2 DS3231 RTC with ESP-IDF
```cpp
// TimeManager_idf.cpp (RTC addition)
#include "ds3231.h"

class RTCManager {
    i2c_dev_t ds3231_dev;

    void init() {
        ds3231_init_desc(&ds3231_dev, I2C_NUM_0,
                        GPIO_NUM_21, GPIO_NUM_22);
    }

    void setTime(struct tm *time) {
        ds3231_set_time(&ds3231_dev, time);
    }

    void getTime(struct tm *time) {
        ds3231_get_time(&ds3231_dev, time);
    }
};
```

---

## Phase 4: Web Configuration (Week 3)
**Goal**: Replace ConfigAssist with native ESP-IDF solution

### Day 17-19: HTTP Server Setup

#### 4.1 ESP-IDF HTTP Server

```cpp
// WebConfigManager.cpp
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

class WebConfigManager {
    httpd_handle_t server = NULL;
    nvs_handle_t nvs_handle;

    // Configuration structure
    struct Config {
        char wifi_ssid[32];
        char wifi_pass[64];
        uint16_t device_id;
        uint16_t measurement_period;
        bool modbus_enabled;
        bool mqtt_enabled;
        char mqtt_broker[128];
    } config;

    static esp_err_t index_handler(httpd_req_t *req) {
        // Serve HTML page
        FILE *f = fopen("/spiffs/index.html", "r");
        if (!f) {
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }

        char buf[1024];
        size_t read_len;
        do {
            read_len = fread(buf, 1, sizeof(buf), f);
            httpd_resp_send_chunk(req, buf, read_len);
        } while (read_len > 0);

        fclose(f);
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }

    static esp_err_t api_config_get_handler(httpd_req_t *req) {
        WebConfigManager* self = (WebConfigManager*)req->user_ctx;

        char json[512];
        snprintf(json, sizeof(json),
            "{\"wifi_ssid\":\"%s\",\"device_id\":%d,"
            "\"measurement_period\":%d,\"modbus_enabled\":%s,"
            "\"mqtt_enabled\":%s,\"mqtt_broker\":\"%s\"}",
            self->config.wifi_ssid, self->config.device_id,
            self->config.measurement_period,
            self->config.modbus_enabled ? "true" : "false",
            self->config.mqtt_enabled ? "true" : "false",
            self->config.mqtt_broker);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    static esp_err_t api_config_post_handler(httpd_req_t *req) {
        WebConfigManager* self = (WebConfigManager*)req->user_ctx;

        char buf[512];
        int ret = httpd_req_recv(req, buf, sizeof(buf));
        if (ret <= 0) {
            return ESP_FAIL;
        }

        // Parse JSON and update config
        cJSON *root = cJSON_Parse(buf);
        if (root) {
            cJSON *item = cJSON_GetObjectItem(root, "wifi_ssid");
            if (item) strncpy(self->config.wifi_ssid, item->valuestring, 32);

            item = cJSON_GetObjectItem(root, "device_id");
            if (item) self->config.device_id = item->valueint;

            // ... parse other fields ...

            cJSON_Delete(root);

            // Save to NVS
            self->saveConfig();

            httpd_resp_send(req, "OK", 2);
        }
        return ESP_OK;
    }

public:
    void init() {
        // Initialize NVS
        nvs_flash_init();
        nvs_open("config", NVS_READWRITE, &nvs_handle);

        // Load config from NVS
        loadConfig();

        // Start HTTP server
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.max_uri_handlers = 20;

        if (httpd_start(&server, &config) == ESP_OK) {
            // Register URI handlers
            httpd_uri_t index_uri = {
                .uri = "/",
                .method = HTTP_GET,
                .handler = index_handler,
                .user_ctx = this
            };
            httpd_register_uri_handler(server, &index_uri);

            httpd_uri_t api_get_uri = {
                .uri = "/api/config",
                .method = HTTP_GET,
                .handler = api_config_get_handler,
                .user_ctx = this
            };
            httpd_register_uri_handler(server, &api_get_uri);

            httpd_uri_t api_post_uri = {
                .uri = "/api/config",
                .method = HTTP_POST,
                .handler = api_config_post_handler,
                .user_ctx = this
            };
            httpd_register_uri_handler(server, &api_post_uri);
        }
    }

    void loadConfig() {
        size_t length = sizeof(config);
        nvs_get_blob(nvs_handle, "config", &config, &length);
    }

    void saveConfig() {
        nvs_set_blob(nvs_handle, "config", &config, sizeof(config));
        nvs_commit(nvs_handle);
    }
};
```

### Day 20-21: Web UI Migration

#### 4.2 Static Web Files
Convert existing web files to work with ESP-IDF HTTP server:

```javascript
// app.js - Update API calls
async function loadConfig() {
    const response = await fetch('/api/config');
    const config = await response.json();

    document.getElementById('wifi_ssid').value = config.wifi_ssid;
    document.getElementById('device_id').value = config.device_id;
    // ... populate other fields
}

async function saveConfig() {
    const config = {
        wifi_ssid: document.getElementById('wifi_ssid').value,
        device_id: parseInt(document.getElementById('device_id').value),
        // ... gather other fields
    };

    await fetch('/api/config', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(config)
    });
}
```

---

## Phase 5: Core Logic Migration (Week 3-4)
**Goal**: Migrate main business logic classes

### Day 22-23: Core Classes Update

#### 5.1 Update MeasurementPoint
```cpp
// MeasurementPoint_idf.cpp
class MeasurementPoint {
    // Replace String with std::string or char arrays
    char name[32];
    float currentTemp;
    float minTemp;
    float maxTemp;

    // Update timing
    int64_t lastReadTime;  // esp_timer_get_time()

public:
    void update() {
        int64_t now = esp_timer_get_time() / 1000;  // Convert to ms
        if (now - lastReadTime > 1000) {
            // Read sensor
            lastReadTime = now;
        }
    }
};
```

#### 5.2 Update Alarm
```cpp
// Alarm_idf.cpp
class Alarm {
    // Use FreeRTOS timer for delays
    TimerHandle_t alarmTimer;

    static void alarmTimerCallback(TimerHandle_t xTimer) {
        Alarm* self = (Alarm*)pvTimerGetTimerID(xTimer);
        self->checkState();
    }

public:
    void init() {
        alarmTimer = xTimerCreate("AlarmTimer",
                                  pdMS_TO_TICKS(1000),
                                  pdTRUE,  // Auto-reload
                                  this,    // Timer ID
                                  alarmTimerCallback);
        xTimerStart(alarmTimer, 0);
    }
};
```

### Day 24-26: Main Controller Migration

#### 5.3 TemperatureController Refactor
```cpp
// TemperatureController_idf.cpp
extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

class TemperatureController {
    // Replace millis() timing
    int64_t lastSensorRead;
    int64_t lastDisplayUpdate;

    // Task handles
    TaskHandle_t sensorTaskHandle;
    TaskHandle_t displayTaskHandle;

    static void sensorTask(void* param) {
        TemperatureController* self = (TemperatureController*)param;
        while (1) {
            self->readAllSensors();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    static void displayTask(void* param) {
        TemperatureController* self = (TemperatureController*)param;
        while (1) {
            self->updateDisplay();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

public:
    void init() {
        // Create FreeRTOS tasks
        xTaskCreatePinnedToCore(sensorTask, "sensors", 4096,
                               this, 5, &sensorTaskHandle, 1);
        xTaskCreatePinnedToCore(displayTask, "display", 4096,
                               this, 3, &displayTaskHandle, 1);
    }
};
```

---

## Phase 6: Final Migration (Week 4)
**Goal**: Complete transition to pure ESP-IDF

### Day 27: Main Application

#### 6.1 ESP-IDF Main Entry Point
```cpp
// main_idf.cpp
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "TempController";

// Component instances
TemperatureController* tempController;
WebConfigManager* configManager;
MQTTManager* mqttManager;

extern "C" void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize networking
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize WiFi
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Initialize components
    tempController = new TemperatureController();
    tempController->init();

    configManager = new WebConfigManager();
    configManager->init();

    mqttManager = new MQTTManager();
    mqttManager->init(configManager->getMqttBroker());

    ESP_LOGI(TAG, "Temperature Controller initialized");

    // Main loop (if needed - most work done in tasks)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Print heap info periodically
        ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
    }
}
```

### Day 28: Remove Arduino Framework

#### 6.2 Final platformio.ini
```ini
[env:esp-wrover-kit]
platform = espressif32
board = esp-wrover-kit
framework = espidf  # Pure ESP-IDF!
monitor_speed = 115200
board_build.partitions = huge_app.csv

# ESP-IDF components (managed by idf.py)
# No more lib_deps needed!
```

#### 6.3 Final CMakeLists.txt
```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "components")

project(temperature_controller)

# Main component
idf_component_register(
    SRCS "src/main_idf.cpp"
         "src/TemperatureController_idf.cpp"
         "src/MeasurementPoint_idf.cpp"
         "src/Alarm_idf.cpp"
         "src/Sensor_idf.cpp"
         "src/LoggerManager_idf.cpp"
         "src/ConfigManager_idf.cpp"
         "src/MQTTManager_idf.cpp"
         "src/ModbusServer_idf.cpp"
         "src/IndicatorInterface_idf.cpp"
         "src/TimeManager_idf.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_wifi esp_http_server mqtt
             driver esp_timer esp_adc fatfs
    EMBED_FILES "data/index.html"
                "data/styles.css"
                "data/app.js"
)
```

---

## Testing Strategy

### Unit Testing Framework
```cpp
// test/test_components.cpp
#include "unity.h"

void test_mqtt_connection() {
    MQTTManager mqtt;
    TEST_ASSERT_TRUE(mqtt.init("mqtt://test.mosquitto.org"));
    TEST_ASSERT_TRUE(mqtt.waitForConnection(5000));
}

void test_sensor_reading() {
    DS18B20Sensor sensor;
    sensor.init(GPIO_NUM_4);
    float temp = sensor.readTemperature();
    TEST_ASSERT_FLOAT_WITHIN(50.0, 20.0, temp);  // -30 to 70°C range
}

void app_main() {
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_connection);
    RUN_TEST(test_sensor_reading);
    UNITY_END();
}
```

### Integration Testing
1. **Component Isolation**: Test each migrated component with mock interfaces
2. **System Integration**: Test component interactions
3. **Performance Validation**: Measure timing and memory usage
4. **Stress Testing**: 24-hour runs with all sensors active

### Regression Testing Checklist
- [ ] All 60 temperature points reading correctly
- [ ] Alarms triggering at correct thresholds
- [ ] MQTT publishing at configured intervals
- [ ] Modbus registers accessible
- [ ] Web configuration saving/loading
- [ ] SD card logging functional
- [ ] OLED display updating
- [ ] LED indicators working
- [ ] Button inputs responsive
- [ ] Memory usage stable over time

---

## Rollback Plan

### Component-Level Rollback
Each component maintains both implementations during migration:

```cpp
// Conditional compilation
#ifdef USE_ESPIDF_IMPL
    #include "MQTTManager_idf.cpp"
#else
    #include "MQTTManager.cpp"
#endif
```

### Full System Rollback
```bash
# If critical issues arise
git checkout arduino-stable
pio run -t upload
```

---

## Risk Mitigation

### High-Risk Areas
1. **ConfigAssist Replacement**
   - Mitigation: Keep Arduino version until Phase 4
   - Fallback: Use IotWebConf as alternative

2. **OneWire Timing**
   - Mitigation: Use RMT-based implementation
   - Fallback: Keep Arduino library in mixed mode

3. **Memory Management**
   - Mitigation: Monitor heap usage continuously
   - Fallback: Increase task stack sizes

### Performance Monitoring
```cpp
// Add performance metrics
class PerformanceMonitor {
    struct Metrics {
        uint32_t heap_free;
        uint32_t heap_min_free;
        uint32_t loop_time_max;
        uint32_t sensor_read_time;
        uint32_t mqtt_publish_time;
    } metrics;

    void logMetrics() {
        ESP_LOGI(TAG, "Heap: %d/%d, Loop: %dms, Sensor: %dms",
                 metrics.heap_free, metrics.heap_min_free,
                 metrics.loop_time_max, metrics.sensor_read_time);
    }
};
```

---

## Success Criteria

### Phase Completion Criteria
Each phase must meet these criteria before proceeding:

1. **Functionality**: All features working as before
2. **Performance**: No degradation in response times
3. **Stability**: 24-hour test without crashes
4. **Memory**: No memory leaks detected
5. **Testing**: All unit tests passing

### Final Acceptance Criteria
- [ ] System runs on pure ESP-IDF framework
- [ ] All Arduino dependencies removed
- [ ] Memory usage reduced by >15KB
- [ ] Boot time improved by >20%
- [ ] All original features functional
- [ ] Documentation updated
- [ ] Team trained on ESP-IDF

---

## Resource Requirements

### Development Environment
- PlatformIO with ESP-IDF support
- ESP-IDF v5.0+ installed
- Serial monitor for debugging
- Logic analyzer for timing verification

### Time Allocation
- **Developer**: 1 senior developer full-time
- **Testing**: 20% time for continuous testing
- **Code Review**: 2 hours per phase

### Hardware Requirements
- Development board with debugging capability
- Test setup with all 60 sensors
- MQTT broker for testing
- Modbus master for testing

---

## Documentation Updates

### Code Documentation
```cpp
/**
 * @brief Temperature Controller ESP-IDF Implementation
 * @note Migrated from Arduino framework
 * @version 2.0.0
 *
 * Changes from v1.x:
 * - Native ESP-IDF framework
 * - FreeRTOS task management
 * - Native MQTT client
 * - NVS-based configuration
 */
```

### API Documentation
- Update all REST API endpoint documentation
- Document new configuration structure
- Update MQTT topic documentation
- Document Modbus register changes

### User Guide Updates
- Configuration interface changes
- New features available in ESP-IDF
- Performance improvements
- Troubleshooting guide

---

## Appendix A: Helper Scripts

### Migration Helper Script
```python
#!/usr/bin/env python3
# migrate_helper.py

import re
import sys

def convert_file(filename):
    """Convert Arduino code patterns to ESP-IDF"""

    replacements = [
        # Logging
        (r'Serial\.printf\((.*?)\);', r'ESP_LOGI(TAG, \1);'),
        (r'Serial\.print\("(.*?)"\);', r'ESP_LOGI(TAG, "\1");'),

        # Timing
        (r'millis\(\)', r'(esp_timer_get_time() / 1000)'),
        (r'delay\((\d+)\)', r'vTaskDelay(pdMS_TO_TICKS(\1))'),
        (r'delayMicroseconds\((\d+)\)', r'ets_delay_us(\1)'),

        # GPIO
        (r'digitalWrite\((.*?),\s*HIGH\)', r'gpio_set_level(\1, 1)'),
        (r'digitalWrite\((.*?),\s*LOW\)', r'gpio_set_level(\1, 0)'),
        (r'digitalRead\((.*?)\)', r'gpio_get_level(\1)'),
        (r'pinMode\((.*?),\s*OUTPUT\)', r'gpio_set_direction(\1, GPIO_MODE_OUTPUT)'),
        (r'pinMode\((.*?),\s*INPUT\)', r'gpio_set_direction(\1, GPIO_MODE_INPUT)'),

        # String class
        (r'String\s+(\w+);', r'char \1[256];'),
        (r'String\s+(\w+)\s*=\s*"(.*?)";', r'char \1[256] = "\2";'),
    ]

    with open(filename, 'r') as f:
        content = f.read()

    for pattern, replacement in replacements:
        content = re.sub(pattern, replacement, content)

    # Add ESP-IDF headers
    if '#include <Arduino.h>' in content:
        content = content.replace('#include <Arduino.h>',
                                  '#include "esp_log.h"\n'
                                  '#include "esp_system.h"\n'
                                  '#include "freertos/FreeRTOS.h"\n'
                                  '#include "freertos/task.h"')

    return content

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python migrate_helper.py <filename>")
        sys.exit(1)

    result = convert_file(sys.argv[1])
    print(result)
```

### Build Script
```bash
#!/bin/bash
# build_espidf.sh

echo "Building ESP-IDF project..."

# Clean build
idf.py fullclean

# Build
idf.py build

# Flash if successful
if [ $? -eq 0 ]; then
    echo "Build successful, flashing..."
    idf.py flash monitor
else
    echo "Build failed!"
    exit 1
fi
```

---

## Appendix B: Common Migration Patterns

### Pattern Reference Table

| Arduino Pattern | ESP-IDF Equivalent | Notes |
|----------------|-------------------|--------|
| `String str = "text"` | `char str[256] = "text"` | Fixed buffer |
| `str += "more"` | `strcat(str, "more")` | Check buffer size |
| `str.length()` | `strlen(str)` | Standard C |
| `WiFi.begin()` | `esp_wifi_start()` | More control |
| `EEPROM.write()` | `nvs_set_*()` | Type-specific |
| `attachInterrupt()` | `gpio_install_isr_service()` | More flexible |
| `analogRead()` | `adc1_get_raw()` | Direct ADC access |
| `yield()` | `vTaskDelay(1)` | FreeRTOS |

---

## Appendix C: Troubleshooting Guide

### Common Issues and Solutions

| Issue | Symptoms | Solution |
|-------|----------|----------|
| **Heap fragmentation** | Crashes after hours | Use static allocation |
| **Stack overflow** | Task crashes | Increase stack size in xTaskCreate |
| **Watchdog timeout** | System resets | Add vTaskDelay() in loops |
| **WiFi not connecting** | No IP address | Check esp_wifi event handlers |
| **MQTT disconnects** | Frequent reconnects | Increase keepalive, check QoS |
| **I2C timeout** | Sensor read fails | Check pull-up resistors |
| **Flash write fails** | NVS errors | Check partition table |
| **Timer not firing** | Callbacks not called | Check timer period and start |

---

## Conclusion

This migration plan provides a systematic approach to transitioning from Arduino to ESP-IDF framework. The incremental approach minimizes risk while allowing the team to learn ESP-IDF gradually. Expected benefits include improved performance, reduced memory usage, and access to professional-grade ESP32 features.

### Key Success Factors
1. **Incremental approach** - One component at a time
2. **Continuous testing** - Verify each phase
3. **Fallback options** - Can revert if needed
4. **Documentation** - Keep everything documented
5. **Team training** - Learn ESP-IDF concepts

### Next Steps
1. Review and approve migration plan
2. Set up development environment
3. Create feature branch for migration
4. Begin Phase 1 implementation
5. Schedule daily progress reviews

---

*Document maintained by: Development Team*
*Last updated: 2024-10-20*
*Version: 1.0*