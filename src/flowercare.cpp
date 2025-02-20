#include "flowercare.h"
#include "baseconfig.h"

FlowerCare::FlowerCare() : previousMillis(0), previousBatteryMillis(0) {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    ScanBLE();
}

void FlowerCare::ScanBLE() {
    BLEScanResults foundDevices = pBLEScan->start(5, false);
    for (int i = 0; i < foundDevices.getCount(); ++i) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        if (device.haveServiceUUID() && device.getServiceUUID().equals(BLEUUID("0000fe95-0000-1000-8000-00805f9b34fb"))) {
            addDevice(device.getAddress().toString());
        }
    }
    pBLEScan->clearResults();
}

void FlowerCare::addDevice(std::string address) {
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
    }
}

void FlowerCare::updateDeviceData(FlowerCareDevice& device) {
    BLEClient* pClient = BLEDevice::createClient();
    if (pClient->connect(BLEAddress(device.address.c_str()))) {
        BLERemoteService* pRemoteService = pClient->getService(BLEUUID("00001204-0000-1000-8000-00805f9b34fb"));
        if (pRemoteService) {
            // Send real-time data read request
            BLERemoteCharacteristic* pWriteCharacteristic = pRemoteService->getCharacteristic(BLEUUID("00001a00-0000-1000-8000-00805f9b34fb"));
            if (pWriteCharacteristic) {
                uint8_t requestData[2] = {0xA0, 0x1F};
                pWriteCharacteristic->writeValue(requestData, 2, true);
            }

            // Read sensor data
            BLERemoteCharacteristic* pReadCharacteristic = pRemoteService->getCharacteristic(BLEUUID("00001a01-0000-1000-8000-00805f9b34fb"));
            if (pReadCharacteristic) {
                std::string value = pReadCharacteristic->readValue();
                device.temperature = (value[0] | (value[1] << 8)) / 10.0;
                device.brightness = value[3] | (value[4] << 8) | (value[5] << 16) | (value[6] << 24);
                device.moisture = value[7];
                device.fertility = value[8] | (value[9] << 8);
            }
        }
        pClient->disconnect();
    }
    delete pClient;
}

void FlowerCare::updateBatteryLevel(FlowerCareDevice& device) {
    BLEClient* pClient = BLEDevice::createClient();
    if (pClient->connect(BLEAddress(device.address.c_str()))) {
        BLERemoteService* pRemoteService = pClient->getService(BLEUUID("00001204-0000-1000-8000-00805f9b34fb"));
        if (pRemoteService) {
            // Read battery level and firmware version
            BLERemoteCharacteristic* pBatteryCharacteristic = pRemoteService->getCharacteristic(BLEUUID("00001a02-0000-1000-8000-00805f9b34fb"));
            if (pBatteryCharacteristic) {
                std::string batteryValue = pBatteryCharacteristic->readValue();
                device.battery = batteryValue[0];
                device.firmwareVersion = batteryValue.substr(2, 4);
            }
        }
        pClient->disconnect();
    }
    delete pClient;
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