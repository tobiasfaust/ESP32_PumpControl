#include "flowercare.h"
FlowerCare::FlowerCare() : previousMillis(0), previousBatteryMillis(0), scanCallbacksInstance(*this) {
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(&scanCallbacksInstance, false); // Set the callback for when devices are discovered, no duplicates.
    pBLEScan->setActiveScan(true);
    ScanBLE();
}

void FlowerCare::ScanBLE() {
    bool scanStarted = pBLEScan->start(5000, false);
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
            delay(500);
            if (pWriteCharacteristic) {
                uint8_t requestData[2] = {0xA0, 0x1F};
                if (pWriteCharacteristic->writeValue(requestData, 2, true)) {
                    Serial.printf("Sent real-time data read request to %s\n", device.address.toString().c_str());
                    
                    NimBLERemoteCharacteristic* pReadCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a01-0000-1000-8000-00805f9b34fb"));
                    if (pReadCharacteristic) {
                        std::string value = pReadCharacteristic->readValue();
                        const char* val = value.c_str();
                        this->printDebugHexValue(val, 16);
                        
                        device.temperature = (float)(val[0] | (val[1] << 8)) / 10.0;
                        device.moisture = val[7];
                        device.brightness = val[3] | (val[4] << 8) | (val[5] << 16) | (val[6] << 24);
                        device.fertility = val[8] | (val[9] << 8);

                        Serial.printf("Temperature: %.1f\n", device.temperature);
                        Serial.printf("Brightness: %d\n", device.brightness);
                        Serial.printf("Moisture: %d\n", device.moisture);
                        Serial.printf("Fertility: %d\n", device.fertility);
                    }

                } else {
                    Serial.printf("Failed to send real-time data read request to %s\n", device.address.toString().c_str());
                }
            }
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
              std::string value = pBatteryCharacteristic->readValue();
              const char* val = value.c_str();
              this->printDebugHexValue(val, 7);
              
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

void FlowerCare::printDebugHexValue(const char* value, int len) {
  Serial.printf("DEBUG: Value length n = %d, Hex: ", len);
  for (int i = 0; i < len; i++) {
    Serial.printf("%02x ", (int)value[i]); 
  }
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