#ifndef FLOWERCARE_H
#define FLOWERCARE_H

#include <vector>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

class FlowerCareDevice {
public:
    std::string address;
    int battery;
    int brightness;
    float temperature;
    int moisture;
    int fertility;
    std::string firmwareVersion;

    FlowerCareDevice(std::string addr) : address(addr), battery(0), brightness(0), temperature(0.0), moisture(0), fertility(0), firmwareVersion("") {}
};

class FlowerCare {
public:
    FlowerCare();
    void ScanBLE();
    void ReadSensors();
    void loop();

private:
    BLEScan* pBLEScan;
    std::vector<FlowerCareDevice> devices;
    unsigned long previousMillis;
    unsigned long previousBatteryMillis;
    const unsigned long interval = 5 * 60 * 1000; // 5 minutes

    void addDevice(std::string address);
    void updateDeviceData(FlowerCareDevice& device);
    void updateBatteryLevel(FlowerCareDevice& device);
};

#endif // FLOWERCARE_H