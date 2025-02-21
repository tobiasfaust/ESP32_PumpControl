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
    void addDevice(NimBLEAddress address);
    void loop();

private:
    NimBLEScan* pBLEScan;
    std::vector<FlowerCareDevice> devices;
    unsigned long previousMillis;
    unsigned long previousBatteryMillis;
    const unsigned long interval = 1 * 60 * 1000; // 5 minutes

    class scanCallbacks : public NimBLEScanCallbacks {
        public:
            scanCallbacks(FlowerCare& flowerCare) : flowerCare(flowerCare) {}
    
            /** Initial discovery, advertisement data only. */
            void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
                if (advertisedDevice->haveServiceUUID() && advertisedDevice->getServiceUUID().equals(NimBLEUUID("0000fe95-0000-1000-8000-00805f9b34fb"))) {
                    flowerCare.addDevice(advertisedDevice->getAddress());
                }
            }
    
            /** onScanEnd */
            void onScanEnd(const NimBLEScanResults& results, int reason) override {
                flowerCare.ReadSensors();
                //flowerCare.ReadBatteryLevels();
            }
    
        private:
            FlowerCare& flowerCare;
    };
    
    scanCallbacks scanCallbacksInstance;

    void updateDeviceData(FlowerCareDevice& device);
    void updateBatteryLevel(FlowerCareDevice& device);

    void printDebugHexValue(const char* value, int len);
};

#endif // FLOWERCARE_H