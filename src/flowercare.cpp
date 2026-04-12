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
        FlowerCare::log(3, "Scanning for FlowerCare devices...");
    }
}

void FlowerCare::addDevice(NimBLEAddress address) {
    for (auto& device : devices) {
        if (device.address == address) {
            return;
        }
    }
    FlowerCare::log(3, "Adding device: %s", address.toString().c_str());
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

void FlowerCare::onLog(std::function<void(int, const char*)> logCallback) {
    this->onlogCallback = logCallback;
}

void FlowerCare::log(int loglevel, const char* format, ...) {
    if (this->onlogCallback) {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        onlogCallback(loglevel, buffer);
        va_end(args);
    }
}

void FlowerCare::onScanEnd(std::function<void()> OnScanEndCallback) {
    this->OnScanEndCallback = OnScanEndCallback;
}

void FlowerCare::ReadSensor(FlowerCareDevice& device, bool getBatteryLevel) {
    bool success = false;
    device.lastRead = millis();

    NimBLEClient* pClient = NimBLEDevice::createClient();
    FlowerCare::log(4, "Connecting to %s for updating data (%d bytes free Heap)", device.address.toString().c_str(), ESP.getFreeHeap());
    
    if (pClient->connect(device.address)) {
        NimBLERemoteService* pRemoteService = pClient->getService(NimBLEUUID("00001204-0000-1000-8000-00805f9b34fb"));
        if (pRemoteService) {
            JsonDocument json;
            json["address"] = device.address.toString();
            // Send real-time data read request
            success = this->updateDeviceData(json, device, pRemoteService);
            // get battery data
            if (success && getBatteryLevel) this->updateBatteryLevel(json, device, pRemoteService);

            // sending over MQTT
            if (mqtt) {
                json["host"] = Config->GetMqttRoot();
                String topic = "flowercare/" + json["address"].as<String>();
                FlowerCare::log(4, "Sending FlowerCare data to MQTT: %s -> %s", topic.c_str(), json.as<String>().c_str());
                mqtt->Publish_String(topic.c_str(), json.as<String>(), true);
            }
  
            // Send data to callback function, if defined
            if (this->onValuesCallback) {
                this->onValuesCallback(json);
            }
        } else {
            FlowerCare::log(1, "Failed to get service from %s", device.address.toString().c_str());
        }
    } else {
        FlowerCare::log(2, "Failed to connect to %s for updating live data", device.address.toString().c_str());
    }

    NimBLEDevice::deleteClient(pClient);

    if (!success) {
        device.failedReads++;
        if (device.failedReads >= this->maxFailedReads) {
            device.active = false;
            FlowerCare::log(2, "Marking device %s as inactive", device.address.toString().c_str());
        } else {
            FlowerCare::log(2, "%d retries left of %d before marking device %s as inactive", this->maxFailedReads - device.failedReads, this->maxFailedReads, device.address.toString().c_str());
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
            FlowerCare::log(4, "Sent real-time data read request to %s", device.address.toString().c_str());
                    
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
                FlowerCare::log(1, "Failed to get characteristics for reading from %s", device.address.toString().c_str());
            }
        } else {
            FlowerCare::log(1, "Failed to send real-time data read request to %s", device.address.toString().c_str());
        }
    } else {
        FlowerCare::log(1, "Failed to get characteristics for writing from %s", device.address.toString().c_str());
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
        FlowerCare::log(1, "Failed to get characteristics from %s", device.address.toString().c_str());
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
  FlowerCare::log(4, "Value length n = %d, Hex: %s", len, str.c_str());
}

bool FlowerCare::setActive(String macaddress, bool active) {
    for (auto& device : devices) {
        if (device.address.toString() == macaddress.c_str()) {
            device.active = active;
            FlowerCare::log(3, "Setting device %s to active: %d", macaddress.c_str(), active);
            return true;
        }
    }
    return false;
}

void FlowerCare::loop() {
    unsigned long currentMillis = millis();
    if (!this->isScanActive && currentMillis - previousMillis >= 2000) { // check every second to do a job
        previousMillis = currentMillis;
        
        for (auto& device : devices) {
            if (device.active && (device.lastRead == 0 || currentMillis - device.lastRead >= this->LiveDataInterval)) {                
                if (device.lastBatteryUpdate == 0 || millis() - device.lastBatteryUpdate >= this->batteryInterval) {
                    this->ReadSensor(device, true);
                } else {
                    this->ReadSensor(device, false);
                }
                break; // only one device per loop
            }
        }
    }
}

//#########################################################################################
flowercareWeb::flowercareWeb(fs::LittleFSFS& configFS) : FlowerCare(), configFS(configFS) {
    this->LoadJsonConfig();
}

void flowercareWeb::GetInitData(JsonDocument& json) {
  const std::vector<FlowerCareDevice>* devices = FlowerCare::getDevices();
  if (devices->size() > 0) {
    JsonArray f = json["data"]["flowercare"].to<JsonArray>();
    for (auto& device : *devices) {
      JsonObject o = f.add<JsonObject>();
      o["address"] = device.address.toString();
      o["mqtttopic"] = String("flowercare/") + String(device.address.toString().c_str());
      o["battery"]["innerHTML"] = device.battery;
      o["battery"]["data-id"] = String(device.address.toString().c_str()) + "_bat";
      o["firmwareVersion"]["innerHTML"] = device.firmwareVersion;
      o["firmwareVersion"]["data-id"] = String(device.address.toString().c_str()) + "_fw";
      o["temperature"]["innerHTML"] = device.temperature;
      o["temperature"]["data-id"] = String(device.address.toString().c_str()) + "_temp";
      o["moisture"]["innerHTML"] = device.moisture;
      o["moisture"]["data-id"] = String(device.address.toString().c_str()) + "_moist";
      o["brightness"]["innerHTML"] = device.brightness;
      o["brightness"]["data-id"] = String(device.address.toString().c_str()) + "_bright";
      o["fertility"]["innerHTML"] = device.fertility;
      o["fertility"]["data-id"] = String(device.address.toString().c_str()) + "_fert";
      o["lastLiveDataUpdate"]["innerHTML"] = device.lastLiveDataUpdate;
      o["lastLiveDataUpdate"]["data-id"] = String(device.address.toString().c_str()) + "_liveupd";
      o["lastBatteryUpdate"]["innerHTML"] = device.lastBatteryUpdate;
      o["lastBatteryUpdate"]["data-id"] = String(device.address.toString().c_str()) + "_batupd";
      o["failedReads"] = device.failedReads;
      o["active"]["checked"] = device.active;
      o["active"]["data-mac"] = device.address.toString();
    }
  }

  json["data"]["fc_devices_table"]["className"] = "editorDemoTable"; // remove class "hide"
  json["data"]["btn_scan"]["className"] = ""; //remove class "hide"

  json["js"]["esp_uptime"] = millis();

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

void flowercareWeb::LoadJsonConfig() {
  if (configFS.exists("/flowercare.json")) {
    //file exists, reading and loading
    FlowerCare::log(3, "reading flowercare.json file....");
    File configFile = configFS.open("/flowercare.json", "r");
    if (configFile) {
      FlowerCare::log(3, "flowercare.json is now open");
 
      ReadBufferingStream stream{configFile, 64};
      stream.find("\"data\":[");
      do {
        JsonDocument elem;
        DeserializationError error = deserializeJson(elem, stream); 
 
        if (error) {
           FlowerCare::log(1, "Failed to parse flowercare.json data: %s", error.c_str()); 
        } else {
          // Print the result
          FlowerCare::log(3, "parsing JSON ok");
          FlowerCare::log(4, elem);
 
          #ifdef USE_FLOWERCARE
          if (elem["address"]) {
            // activation of known FlowerCare devices
            FlowerCare::addDevice(NimBLEAddress(elem["address"].as<String>().c_str(), BLE_ADDR_PUBLIC));
            FlowerCare::setActive(elem["address"].as<String>(), elem["active"].as<bool>());
          }
          #endif
        }
      } while (stream.findUntil(",","]"));
    } else {
      FlowerCare::log(1, "failed to load flowercare.json");
    }
  } else {
    FlowerCare::log(3, "flowercare.json File not exists");
  }
}