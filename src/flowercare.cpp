#include "flowercare.h"
#include "baseconfig.h"

FlowerCare::FlowerCare() : previousMillis(0), previousBatteryMillis(0) {
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    ScanBLE();
}

void FlowerCare::ScanBLE() {
    bool scanStarted = pBLEScan->start(5, false);
    if (scanStarted) {
        NimBLEScanResults foundDevices = pBLEScan->getResults();
        for (int i = 0; i < foundDevices.getCount(); ++i) {
            const NimBLEAdvertisedDevice* device = foundDevices.getDevice(i);
            if (device->haveServiceUUID() && device->getServiceUUID().equals(NimBLEUUID("0000fe95-0000-1000-8000-00805f9b34fb"))) {
                addDevice(device->getAddress());
            }
        }
        pBLEScan->clearResults();
    }
}

void FlowerCare::addDevice(NimBLEAddress address) {
    for (auto& device : devices) {
        if (device.address == address) {
            return;
        }
    }
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
    if (pClient->connect(device.address)) {
        NimBLERemoteService* pRemoteService = pClient->getService(NimBLEUUID("00001204-0000-1000-8000-00805f9b34fb"));
        if (pRemoteService) {
            // Send real-time data read request
            NimBLERemoteCharacteristic* pWriteCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a00-0000-1000-8000-00805f9b34fb"));
            if (pWriteCharacteristic) {
                uint8_t requestData[2] = {0xA0, 0x1F};
                pWriteCharacteristic->writeValue(requestData, 2, true);
            }

            // Read sensor data
            NimBLERemoteCharacteristic* pReadCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID("00001a01-0000-1000-8000-00805f9b34fb"));
            if (pReadCharacteristic) {
                String value = pReadCharacteristic->readValue().c_str();
                device.temperature = (value[0] | (value[1] << 8)) / 10.0;
                device.brightness = value[3] | (value[4] << 8) | (value[5] << 16) | (value[6] << 24);
                device.moisture = value[7];
                device.fertility = value[8] | (value[9] << 8);
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
                String batteryValue = pBatteryCharacteristic->readValue().c_str();
                if (batteryValue.length() >= 4) {
                    // Convert hex string to battery level and firmware version
                    device.battery = static_cast<int>(strtol(batteryValue.substring(0, 2).c_str(), nullptr, 16));
                    device.firmwareVersion = String(static_cast<char>(strtol(batteryValue.substring(2, 4).c_str(), nullptr, 16)));
                }
            }
        }
    }
    NimBLEDevice::deleteClient(pClient);
}

void FlowerCare::loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        ReadSensors();
    }

    // Update battery level and firmware version every hour
    if (currentMillis - previousBatteryMillis >= 3600000) { // 1 hour in milliseconds
        previousBatteryMillis = currentMillis;
        for (auto& device : devices) {
            updateBatteryLevel(device);
        }
    }
}