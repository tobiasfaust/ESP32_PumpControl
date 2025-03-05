#ifndef FLOWERCARE_H
#define FLOWERCARE_H

#include <vector>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>

class FlowerCareDevice {
 public:
    NimBLEAddress address;
    bool active;
    int battery;
    int brightness;
    float temperature;
    int moisture;
    int fertility;
    String firmwareVersion;
    unsigned long lastLiveDataUpdate;
    unsigned long lastBatteryUpdate;
    uint8_t failedReads;
    unsigned long lastRead;

    FlowerCareDevice(NimBLEAddress addr) : 
        address(addr),
        active(true),
        battery(0),
        brightness(0),
        temperature(0.0),
        moisture(0),
        fertility(0),
        firmwareVersion(""),
        lastLiveDataUpdate(0),
        lastBatteryUpdate(0),
        lastRead(0),
        failedReads(0)
        {}
};

class FlowerCare {
  public:

    /************************
     * @brief Constructor
     ************************/
    FlowerCare();

    /************************
     * @brief loop function
     ************************/
    void loop();

    /************************
     * @brief start the scan for BLE devices
     ************************/
    void ScanBLE();

    /************************
     * @brief add a device to the list
     * @param NimBLEAddress the address of the device
     ************************/
    void addDevice(NimBLEAddress address);

    /************************
     * @brief set the active state of a device
     * @param String the mac address of the device like c4:7c:8d:64:42:d0
     * @param bool set the active state
     * @return bool true if successful
     ************************/
    bool setActive(String macaddress, bool active);

    /************************
     * @brief get the device by address
     * @param NimBLEAddress the address of the device
     ************************/
    const FlowerCareDevice* getDevice(NimBLEAddress address);

    /************************
     * @brief get all devices in a vector
     * @return std::vector<FlowerCareDevice> the devices
     ************************/
    const std::vector<FlowerCareDevice>* getDevices() const { return &devices; }

    /************************
     * @brief get the active state of the scan
     * @return bool the active state
     ************************/
    const bool& getIsScanActive() const { return isScanActive; }
  
    // callbacks
    /************************
     * @brief Callback for getting the values
     * @param function(JsonDocument&) the callback function
     ************************/
    void onValues(std::function<void(JsonDocument&)> callback);

    /************************
     * @brief Callback for logging
     * @param function(const int, const char*, ...) the callback function
     ************************/
    void onLog(std::function<void(int, const char*)> onlogCallback);

    /************************
     * @brief Callback for scan end
     * @param function() the callback function
     ************************/
    void onScanEnd(std::function<void()> OnScanEndCallback);
    
  protected:
    std::function<void()> OnScanEndCallback; // Callback function pointer

  private:

    NimBLEScan* pBLEScan;
    std::vector<FlowerCareDevice> devices;

    unsigned long previousMillis;
    const unsigned long LiveDataInterval = 5 * 60 * 1000; // default: 5 minutes
    const unsigned long batteryInterval =  60 * 60 * 1000; // default: 1 hour
    const uint8_t maxFailedReads = 5; // Number of failed continously reads before marking device as inactive
    bool isScanActive;

    class scanCallbacks : public NimBLEScanCallbacks {
        public:
            scanCallbacks(FlowerCare& flowerCare) : flowerCare(flowerCare) {}
    
            /** Initial discovery, advertisement data only. */
            void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
                if (advertisedDevice->haveServiceUUID() && advertisedDevice->getServiceUUID().equals(NimBLEUUID("0000fe95-0000-1000-8000-00805f9b34fb"))) {
                    flowerCare.addDevice(advertisedDevice->getAddress());
                }
            }

            void onScanEnd(const NimBLEScanResults& results, int reason) override {
                flowerCare.isScanActive = false;
                if (flowerCare.OnScanEndCallback) {
                    flowerCare.OnScanEndCallback();
                }
            }
    
        private:
            FlowerCare& flowerCare;
    };
    
    scanCallbacks scanCallbacksInstance;
    
    void ReadSensor(FlowerCareDevice& device, bool getBatteryLevel = false);
    bool updateDeviceData(JsonDocument& json, FlowerCareDevice& device, NimBLERemoteService* pRemoteService);
    bool updateBatteryLevel(JsonDocument& json, FlowerCareDevice& device, NimBLERemoteService* pRemoteService);

    void printDebugHexValue(const char* value, int len);

    void log(int loglevel, const char* format, ...);
    std::function<void(int, const char*)> onlogCallback; // Callback function pointer
    std::function<void(JsonDocument&)> onValuesCallback; // Callback function pointer
};

#endif // FLOWERCARE_H