#include "flowercare.h"

FlowerCare::FlowerCare() : previousMillis(0), isScanActive(false), scanCallbacksInstance(*this) {
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(&scanCallbacksInstance, false); // Set the callback for when devices are discovered, no duplicates.
    pBLEScan->setActiveScan(true);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
}

void FlowerCare::ScanBLE() {
    this->isScanActive = true;
    bool scanStarted = pBLEScan->start(10000, false);
    if (scanStarted) {
        log(3, "Scanning for FlowerCare devices...");
    }
}

void FlowerCare::addDevice(NimBLEAddress address) {
    for (auto& device : devices) {
        if (device.address == address) {
            return;
        }
    }
    log(3, "Adding device: %s", address.toString().c_str());
    devices.emplace_back(address);
}

const FlowerCareDevice* FlowerCare::getDevice(NimBLEAddress address) {
    for (auto& device : devices) {
        if (device.address == address) {
            return &device;
        }
    }
    return nullptr;
}

void FlowerCare::onValues(std::function<void(JsonDocument&)> callback) {
    this->onValuesCallback = callback;
}

void FlowerCare::onLog(std::function<void(int, const char*, va_list)> logCallback) {
    this->onlogCallback = logCallback;
}

void FlowerCare::log(int loglevel, const char* format, ...) {
    if (this->onlogCallback) {
        va_list args;
        va_start(args, format);
        this->onlogCallback(loglevel, format, args);
        va_end(args);
    }
}

void FlowerCare::onScanEnd(std::function<void()> OnScanEndCallback) {
    this->OnScanEndCallback = OnScanEndCallback;
}

void FlowerCare::ReadSensor(FlowerCareDevice& device, bool getBatteryLevel) {
    bool success = false;
    NimBLEClient* pClient = NimBLEDevice::createClient();
    log(4, "Connecting to %s for updating data (%d bytes free Heap)", device.address.toString().c_str(), ESP.getFreeHeap());
    
    if (pClient->connect(device.address)) {
        NimBLERemoteService* pRemoteService = pClient->getService(NimBLEUUID("00001204-0000-1000-8000-00805f9b34fb"));
        if (pRemoteService) {
            JsonDocument json;
            json["address"] = device.address.toString();
            // get battery data
            success = this->updateBatteryLevel(json, device, pRemoteService);
            // Send real-time data read request
            success = this->updateDeviceData(json, device, pRemoteService);
            // Send data to callback function, if defined
            if (this->onValuesCallback) {
                this->onValuesCallback(json);
            }
        } else {
            log(1, "Failed to get service from %s", device.address.toString().c_str());
        }
    } else {
        log(2, "Failed to connect to %s for updating live data", device.address.toString().c_str());
    }

    NimBLEDevice::deleteClient(pClient);

    if (!success) {
        device.failedReads++;
        device.lastLiveDataUpdate = millis();
        if (device.failedReads >= this->maxFailedReads) {
            device.active = false;
            log(2, "Marking device %s as inactive", device.address.toString().c_str());
        } else {
            log(2, "%d retries left of %d before marking device %s as inactive", this->maxFailedReads - device.failedReads, this->maxFailedReads, device.address.toString().c_str());
        }
    } else {
        device.failedReads = 0;
    }
}

bool FlowerCare::updateDeviceData(JsonDocument& json, FlowerCareDevice& device, NimBLERemoteService* pRemoteService) {
    bool ret = false;
    NimBLERemoteCharacteristic* pWriteCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a00-0000-1000-8000-00805f9b34fb"));
    delay(500);
    if (pWriteCharacteristic) {
        uint8_t requestData[2] = {0xA0, 0x1F};
        if (pWriteCharacteristic->writeValue(requestData, 2, true)) {
            log(4, "Sent real-time data read request to %s", device.address.toString().c_str());
                    
            NimBLERemoteCharacteristic* pReadCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a01-0000-1000-8000-00805f9b34fb"));
            if (pReadCharacteristic) {
                device.lastLiveDataUpdate = millis();
                std::string value = pReadCharacteristic->readValue();
                const char* val = value.c_str();
                this->printDebugHexValue(val, 16);
                        
                device.temperature = (float)(val[0] | (val[1] << 8)) / 10.0;
                device.moisture = val[7];
                device.brightness = val[3] | (val[4] << 8) | (val[5] << 16) | (val[6] << 24);
                device.fertility = val[8] | (val[9] << 8);

                json["temperature"] = device.temperature;
                json["moisture"] = device.moisture;
                json["brightness"] = device.brightness;
                json["fertility"] = device.fertility;

                ret = true;

            } else {
                log(1, "Failed to get characteristics for reading from %s", device.address.toString().c_str());
            }
        } else {
            log(1, "Failed to send real-time data read request to %s", device.address.toString().c_str());
        }
    } else {
        log(1, "Failed to get characteristics for writing from %s", device.address.toString().c_str());
    }
    return ret;
}

bool FlowerCare::updateBatteryLevel(JsonDocument& json, FlowerCareDevice& device, NimBLERemoteService* pRemoteService) {
    // Read battery level and firmware version
    bool ret = false;
    NimBLERemoteCharacteristic* pBatteryCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a02-0000-1000-8000-00805f9b34fb"));
    if (pBatteryCharacteristic) {
        device.lastBatteryUpdate = millis();
        std::string value = pBatteryCharacteristic->readValue();
        const char* val = value.c_str();
        this->printDebugHexValue(val, 7);
      
        if (value.length() >= 4) {
            // Convert hex string to battery level and firmware version
            device.battery = (uint8_t)value[0]; 
            device.firmwareVersion = &value[2];

            json["battery"] = device.battery;
            json["firmwareVersion"] = device.firmwareVersion;

            ret = true;
        }
    } else {
        log(1, "Failed to get characteristics from %s", device.address.toString().c_str());
    }
    return ret;
}

void FlowerCare::printDebugHexValue(const char* value, int len) {
  String str; str.reserve(len * 6);
  for (int i = 0; i < len; i++) {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "0x%02x ", (int)value[i]);
    str += buffer;
  }
  log(4, "Value length n = %d, Hex: %s", len, str.c_str());
}

void FlowerCare::setActive(String macaddress, bool active) {
    for (auto& device : devices) {
        if (device.address.toString() == macaddress.c_str()) {
            device.active = active;
            log(3, "Setting device %s to active: %d", macaddress.c_str(), active);
            return;
        }
    }
}

void FlowerCare::loop() {
    unsigned long currentMillis = millis();
    if (!this->isScanActive && currentMillis - previousMillis >= 2000) { // check every second to do a job
        previousMillis = currentMillis;
        
        for (auto& device : devices) {
            if (device.active && (device.lastLiveDataUpdate == 0 || currentMillis - device.lastLiveDataUpdate >= this->LiveDataInterval)) {                
                if (millis() - device.lastBatteryUpdate >= this->batteryInterval) {
                    this->ReadSensor(device, true);
                } else {
                    this->ReadSensor(device, false);
                }
                break; // only one device per loop
            }
        }
    }
}