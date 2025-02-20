#ifndef FLOWERCARE_H
#define FLOWERCARE_H

#include <vector>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <Arduino.h>

class FlowerCareDevice {
public:
    NimBLEAddress address;
    int battery;
    int brightness;
    float temperature;
    int moisture;
    int fertility;
    String firmwareVersion;

    FlowerCareDevice(NimBLEAddress addr) : address(addr), battery(0), brightness(0), temperature(0.0), moisture(0), fertility(0), firmwareVersion("") {}
};

class FlowerCare {
public:
    FlowerCare();
    void ScanBLE();
    void ReadSensors();
    void loop();

private:
    NimBLEScan* pBLEScan;
    std::vector<FlowerCareDevice> devices;
    unsigned long previousMillis;
    unsigned long previousBatteryMillis;
    const unsigned long interval = 5 * 60 * 1000; // 5 minutes

    void addDevice(NimBLEAddress address);
    void updateDeviceData(FlowerCareDevice& device);
    void updateBatteryLevel(FlowerCareDevice& device);
};

#endif // FLOWERCARE_H