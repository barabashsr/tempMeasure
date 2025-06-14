#include "FS.h"
#include "SD.h"
#include "SPI.h"

//SPI PINs
#define SCK_PIN  14
#define MISO_PIN  12
#define MOSI_PIN  13
#define CS5_PIN_TF_CARD  5

#define SD_CS 5  // Chip Select pin

void writeCSVHeader() {
    File file = SD.open("/data.csv", FILE_WRITE);
    if (file) {
        file.println("Date,Time,Temperature,Humidity");
        file.close();
        Serial.println("CSV header created");
    } else {
        Serial.println("Error creating CSV file");
    }
}

void writeCSVData(String date, String time, float temperature, float humidity) {
    File file = SD.open("/data.csv", FILE_APPEND);
    if (file) {
        file.print(date);
        file.print(",");
        file.print(time);
        file.print(",");
        file.print(temperature);
        file.print(",");
        file.println(humidity);
        file.close();
        Serial.println("Data appended to CSV");
    } else {
        Serial.println("Error opening CSV file for writing");
    }
}

void setup() {
    Serial.begin(115200);
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);  // SCK, MISO, MOSI, SS
    
    // Initialize SD card
    if (!SD.begin(CS5_PIN_TF_CARD)) {
        Serial.println("Card Mount Failed");
        return;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
    }
    
    // Print card info
    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
    // Create CSV file with headers if it doesn't exist
    File file = SD.open("/data.csv");
    if (!file) {
        writeCSVHeader();
    }
    file.close();
    
    // Write some sample data
    writeCSVData("2025-06-14", "18:30:00", 25.6, 60.2);
    writeCSVData("2025-06-14", "18:31:00", 25.8, 59.8);
    
    Serial.println("CSV data written successfully!");
}



void loop() {
    // Your main code here
    // You can call writeCSVData() whenever you need to log data
    delay(1000);
}
