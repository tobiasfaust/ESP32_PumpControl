#include "flowercare.h"
FlowerCare::FlowerCare() : previousMillis(0), previousBatteryMillis(0), scanCallbacksInstance(*this) {
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(&scanCallbacksInstance, false); // Set the callback for when devices are discovered, no duplicates.
    pBLEScan->setActiveScan(true);
    ScanBLE();
}

void FlowerCare::ScanBLE() {
    bool scanStarted = pBLEScan->start(10000, false);
    if (scanStarted) {
        Serial.printf("Scanning for FlowerCare devices...\n");
    }
}

void FlowerCare::addDevice(NimBLEAddress address) {
    for (auto& device : devices) {
        if (device.address == address) {
            return;
        }
    }
    Serial.printf("Adding device: %s\n", address.toString().c_str());
    devices.emplace_back(address);
}

void FlowerCare::ReadSensors() {
    for (auto& device : devices) {
        updateDeviceData(device);
        updateBatteryLevel(device);
    }
}

void FlowerCare::updateDeviceData(FlowerCareDevice& device) {
    NimBLEClient* pClient = NimBLEDevice::createClient();
    Serial.printf("Connecting to %s\n", device.address.toString().c_str());
    if (pClient->connect(device.address)) {
        NimBLERemoteService* pRemoteService = pClient->getService(NimBLEUUID("00001204-0000-1000-8000-00805f9b34fb"));
        if (pRemoteService) {
            // Send real-time data read request
            NimBLERemoteCharacteristic* pWriteCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a00-0000-1000-8000-00805f9b34fb"));
            if (pWriteCharacteristic) {
                uint8_t requestData[2] = {0xA0, 0x1F};
                if (pWriteCharacteristic->writeValue(requestData, 2, true)) {
                    Serial.printf("Sent real-time data read request to %s\n", device.address.toString().c_str());
                    
                    NimBLERemoteCharacteristic* pReadCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a01-0000-1000-8000-00805f9b34fb"));
                    if (pReadCharacteristic) {
                        String value = pReadCharacteristic->readValue();
                        this->printDebugHexValue(value, value.length());
                        device.temperature = (value[0] | (value[1] << 8)) / 10.0;
                        device.brightness = value[3] | (value[4] << 8) | (value[5] << 16) | (value[6] << 24);
                        device.moisture = value[7];
                        device.fertility = value[8] | (value[9] << 8);
                        Serial.printf("Temperature: %.1f\n", device.temperature);
                        Serial.printf("Brightness: %d\n", device.brightness);
                        Serial.printf("Moisture: %d\n", device.moisture);
                        Serial.printf("Fertility: %d\n", device.fertility);
                    }

                } else {
                    Serial.printf("Failed to send real-time data read request to %s\n", device.address.toString().c_str());
                }
            }

            // Read sensor data
        /*    NimBLERemoteCharacteristic* pReadCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a01-0000-1000-8000-00805f9b34fb"));
            if (pReadCharacteristic) {
                String value = pReadCharacteristic->readValue();
                this->printDebugHexValue(value, value.length());
                /*device.temperature = (value[0] | (value[1] << 8)) / 10.0;
                device.brightness = value[3] | (value[4] << 8) | (value[5] << 16) | (value[6] << 24);
                device.moisture = value[7];
                device.fertility = value[8] | (value[9] << 8);
                Serial.printf("Temperature: %.1f\n", device.temperature);

                uint8_t data[value.length()];
                value.getBytes(data, value.length());

                String hexString = "";
                for (int i = 0; i < value.length(); i++) {
                    char hex[3];
                    sprintf(hex, "%02X", data[i]);
                    hexString += hex;
                }
                Serial.printf("Hex String: %s\n", hexString.c_str());

                device.temperature = (int16_t)(data[0] | (data[1] << 8)) / 10.0;                    // 2 bytes at pos 00-01: "0E 01"        -> 270 * 0.1°C = 27.0 °C
                                                                                                    // 1 byte at pos 02 unknown, seems to be fixed value of "0x00"
                device.brightness = data[3] | (data[4] << 8) | (data[5] << 16) | (data[6] << 24);   // 4 bytes at pos 03-06: "48 02 00 00"  -> 584 lux
                device.moisture = data[7];                                                          // 1 byte  at pos 07:    "28"           -> 40%
                device.fertility = data[8] | (data[9] << 8);                                        // 2 bytes at pos 08-09: "D0 00"        -> 208 µS/cm
                                                                                                    // 6 bytes at pos 10-15 unknown, seems to be fixed value of "0x02 3C 00 FB 34 9B" 
                // print all values
                Serial.printf("Temperature: %.1f °C\n", device.temperature);
                Serial.printf("Brightness: %d lux\n", device.brightness);
                Serial.printf("Moisture: %d%%\n", device.moisture);
                Serial.printf("Fertility: %d µS/cm\n", device.fertility);                                                                                                    
            } */
        }
    }
    NimBLEDevice::deleteClient(pClient);
}

void FlowerCare::updateBatteryLevel(FlowerCareDevice& device) {
    NimBLEClient* pClient = NimBLEDevice::createClient();
    if (pClient->connect(device.address)) {
        NimBLERemoteService* pRemoteService = pClient->getService(NimBLEUUID("00001204-0000-1000-8000-00805f9b34fb"));
        if (pRemoteService) {
            // Read battery level and firmware version
            NimBLERemoteCharacteristic* pBatteryCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a02-0000-1000-8000-00805f9b34fb"));
            if (pBatteryCharacteristic) {
                String value = pBatteryCharacteristic->readValue();
                this->printDebugHexValue(value, value.length());
                if (value.length() >= 4) {
                    // Convert hex string to battery level and firmware version
                    device.battery = (uint8_t)value[0]; 
                    device.firmwareVersion = &value[2];
                    Serial.printf("Battery: %d%% , Firmware version: %s\n", device.battery, device.firmwareVersion.c_str());
                }
            }
        }
    }
    NimBLEDevice::deleteClient(pClient);
}

void FlowerCare::printDebugHexValue(String value, int len) {
    Serial.printf("DEBUG: Value length n = %d, Hex: ", len);
    for (int i = 0; i < len; i++)
        Serial.printf("%02x ", (int)value[i]); 
    Serial.println(" ");
}


void FlowerCare::loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        ReadSensors();
    }

    // Update battery level and firmware version every hour
    if (currentMillis - previousBatteryMillis >= interval+1000) { // 1 hour in milliseconds
        previousBatteryMillis = currentMillis;
        for (auto& device : devices) {
            updateBatteryLevel(device);
        }
    }
}