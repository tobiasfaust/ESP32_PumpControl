#include "mywebserver.h" 

MyWebServer::MyWebServer(AsyncWebServer *server, DNSServer* dns): 
        server(server), 
        dns(dns), 
        DoReboot(false),
        RequestRebootTime(0) {
  
  fsfiles = new handleFiles(server);
  fsfiles->registerLogCallback(std::bind(&BaseConfig::logN, Config, std::placeholders::_1, std::placeholders::_2));

  _relationen = new std::vector<FlowercareRelation_t>();
  
  #ifdef USE_FLOWERCARE
    Config->logN(1, "Starting FlowerCare");
    flowerCare = new FlowerCare();
    flowerCare->onLog(std::bind(&BaseConfig::logN, Config, std::placeholders::_1, std::placeholders::_2));
    flowerCare->onValues(std::bind(&MyWebServer::flowerCareGetValuesCallback, this, std::placeholders::_1));
    flowerCare->onScanEnd(std::bind(&MyWebServer::flowerCareOnScanEndCallback, this));
  #endif
  
  this->LoadFlowerCareConfig();

  ws = new AsyncWebSocket("/ajaxws");

  ElegantOTA.begin(server);
  ElegantOTA.setGitEnv(String(GIT_OWNER), String(GIT_REPO), String(GIT_BRANCH), String(GITHUB_RUN).toInt());
  ElegantOTA.setFWVersion(String(Config->GetReleaseName() + " / Build: " + GITHUB_RUN ));
  ElegantOTA.setFWVariant(String(GIT_VARIANT));
  ElegantOTA.setAutoReboot(true);
  ElegantOTA.onStart(std::bind(&MyWebServer::onOTAStart, this));
  ElegantOTA.onProgress(std::bind(&MyWebServer::onOTAProgress, this, std::placeholders::_1, std::placeholders::_2));
  ElegantOTA.onEnd(std::bind(&MyWebServer::onOTAEnd, this, std::placeholders::_1));

  server->on("/", HTTP_GET, std::bind(&MyWebServer::handleRoot, this, std::placeholders::_1));
  server->onNotFound(std::bind(&MyWebServer::handleNotFound, this, std::placeholders::_1));
  
  ws->onEvent(std::bind(&MyWebServer::onWsEvent, this, std::placeholders::_1, 
    std::placeholders::_2, 
    std::placeholders::_3, 
    std::placeholders::_4, 
    std::placeholders::_5, 
    std::placeholders::_6 ));

  server->addHandler(ws);
  
  server->serveStatic("/web/", sysFS, "/", "max-age=3600").setDefaultFile("/web/index.html");

  // try to start the server if wifi is connected, otherwise wait for wifi connection
  if (mqtt->GetConnectStatusWifi()) {
    server->begin();
    Config->logN(1, "WebServer has been started ...");
  } else {
    mqtt->improvSerial.onImprovConnected(std::bind(&MyWebServer::onImprovWiFiConnectedCb, this, std::placeholders::_1, std::placeholders::_2));
  }
}

void MyWebServer::onImprovWiFiConnectedCb(const char *ssid, const char *password) {
  server->begin();
  Config->logN(1, "WebServer has been started now ...");
}

void MyWebServer::onOTAStart() {
  // Log when OTA has started
  Config->logN(3, "OTA update started!");
}

void MyWebServer::onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Config->logN(4, "OTA Progress Current: %u bytes, Final: %u bytes", current, final);
  }
}

void MyWebServer::onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Config->logN(3, "OTA update finished successfully!");
  } else {
    Config->logN(3, "There was an error during OTA update!");
  }
}

void MyWebServer::handleRoot(AsyncWebServerRequest *request) {
  request->redirect("/web/index.html");
}

void MyWebServer::loop() {
  //delay(1); // slow response Issue: https://github.com/espressif/arduino-esp32/issues/4348#issuecomment-695115885
  if (this->DoReboot) {
    if (this->RequestRebootTime == 0) {
      this->RequestRebootTime = millis();
      Config->logN(1, "Request to Reboot, wait 5sek ...");
    }
    if (millis() - this->RequestRebootTime > 5000) { // wait 3sek until reboot
      Config->logN(1, "Rebooting...");
      ESP.restart();
    }
  }

  ElegantOTA.loop();
  ws->cleanupClients();

  #ifdef USE_FLOWERCARE
    if (flowerCare) {
      flowerCare->loop();
    }
  #endif

}

void MyWebServer::handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

bool MyWebServer::handleReset() {
  bool ret = true;
  Config->logN(3, "deletion of all config files was requested ....");
  //LittleFS.format(); // Werkszustand -> nur die config dateien loeschen, die register dateien muessen erhalten bleiben
  File root = LittleFS.open("/config/", "w");
  File file = root.openNextFile();
  while(file){
    String path("/config/"); path.concat(file.name());
    if (path.indexOf(".json") == -1) {file = root.openNextFile(); continue;}
    file.close();
    
    if (LittleFS.remove(path)) {
      Config->logN(4, "deletion of configuration file '%s' was successful", file.name());
    } else {
      Config->logN(2, "deletion of configuration file '%s' has failed", file.name());
      ret = false;
    }
    file = root.openNextFile();
  }
  root.close();
  this->DoReboot = true;

  return ret;
}

void MyWebServer::onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Config->logN(4, "[Client: %u] WebSocket client connected", client->id());
  
  } else if (type == WS_EVT_DISCONNECT) {
    Config->logN(4, "[Client: %u] WebSocket client disconnected", client->id());
  
  } else if (type == WS_EVT_DATA) {
    String msg(""); msg.reserve(len + 1);
    for (size_t i = 0; i < len; i++) { msg += (char)data[i]; } msg += '\0';
    Config->logN(4, "[Client: %u] WebSocket data received: %s", client->id(), msg.c_str()); 

    String action(""), subaction(""), item(""), item2("");
    bool newState = false;
    JsonDocument json;
    DeserializationError error = deserializeJson(json, msg.c_str());
    if (!error) {
      if (json["cmd"]) {
        if (json["cmd"]["action"])      { action    = json["cmd"]["action"].as<String>();}
        if (json["cmd"]["subaction"])   { subaction = json["cmd"]["subaction"].as<String>();}
        if (json["cmd"]["newState"])    { newState  = json["cmd"]["newState"].as<bool>();}
        if (json["cmd"]["item"])        { item      = json["cmd"]["item"].as<String>(); }
        if (json["cmd"]["item2"])       { item2     = json["cmd"]["item2"].as<String>(); }
        
      }

      if (action && action == "reset") {
        if (handleReset()) {
          json["response"]["status"] = 1;
          json["response"]["text"] = "all config files deleted successfully";
        } else {
          json["response"]["status"] = 0;
          json["response"]["text"] = "deletion of config files failed";
        }
      }

      if(action && action == "reboot") {
        this->DoReboot = true;
        json["response"]["status"] = 1;
        json["response"]["text"] = "reboot after 5sec...";
      }

      if(action && action == "GetInitData")  {
        if (subaction && subaction == "status") {
          this->GetInitDataStatus(json);
          VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "navi") {
          this->GetInitDataNavi(json);
        } else if (subaction && subaction == "baseconfig") {
          Config->GetInitData(json);
          VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "valveconfig") {
          VStruct->GetInitData(json);
          VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "1wireconfig") {
          VStruct->GetInitData1Wire(json);
          VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "sensorconfig") {
          LevelSensor->GetInitData(json);
          VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "relations") {
          ValveRel->GetInitData(json);
          VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "flowercare") {
          this->GetInitDataFlowerCare(json);
        } else {
          json["response"]["status"] = 0;
          json["response"]["text"] = "unknown subaction";
        }
      }

      if(action && action == "ReloadConfig")  {
        if (subaction && subaction == "baseconfig") {
          Config->LoadJsonConfig();
        } 
        
        if (subaction && subaction == "valveconfig") {
          VStruct->LoadJsonConfig();
        } 
        
        if (subaction && subaction == "sensorconfig") {
          LevelSensor->LoadJsonConfig();
        } 
        
        if (subaction && subaction == "relations") {
          ValveRel->LoadJsonConfig();
        }

        if (subaction && subaction == "flowercare") {
          this->LoadFlowerCareConfig();
        }
      
        json["response"]["status"] = 1;
        json["response"]["text"] = "new config reloaded sucessfully";
      }

      if(action && action == "handlefiles") {
        fsfiles->HandleRequest(json);
      }
     
      if (action && action == "flowercare") {
        #ifdef USE_FLOWERCARE
        if (flowerCare && subaction && subaction == "scan") {
          flowerCare->ScanBLE();
          json["response"]["status"] = 1;
          json["response"]["text"] = "scan started, please wait 10sec .....";
        }

        if (flowerCare && subaction && subaction == "activateDevice") {
          if (flowerCare->setActive(item, newState)) {
            json["response"]["status"] = 1;
            json["response"]["text"] = String("device set to ") + (newState ? "active" : "inactive");
          } else {
            json["response"]["status"] = 0;
            json["response"]["text"] = "device not found";
          }
        }
        #endif
        
        if (subaction && subaction == "activateRelation") {
          if (item && item2 && newState) {
            // item -> mqtttopic, item2 -> port
            for (uint8_t i=0; i<_relationen->size(); i++) {
              if (_relationen->at(i).TriggerTopic == item && _relationen->at(i).ActorPort == item2.toInt()) {
                _relationen->at(i).enabled = newState;
                if (newState) mqtt->Subscribe(item, MyMQTT::FLOWERCARE); // do not unsubscribe, because it is possible that the same topic is used by another relation
                json["response"]["status"] = 1;
                json["response"]["text"] = (newState ? "relation activated" : "relation deactivated");
                break;
              } else {
                json["response"]["status"] = 0;
                json["response"]["text"] = "relation not found, please save first";
              }
            }
          }
        }
      }

      if(action && action == "SetValve") {
        uint8_t port = item.toInt();
        if (item && port > 0 && !VStruct->GetEnabled(port)) { 
          json["response"]["status"] = 0; 
          json["response"]["text"] = "Requested Port not enabled. Please enable first!";
        }
        else if (item && port > 0 )  { 
          if (newState) {
            VStruct->SetOn(port); 
          } else { 
            VStruct->SetOff(port); 
          }

          json["response"]["status"] = 1;
          json["response"]["text"] =(VStruct->GetState(port)?"Valve is now: ON":"Valve is now: OFF");
          json["data"][subaction] = (VStruct->GetState(port)?"Set Off":"Set On"); // subaction = button.id
        }
      }

      if(action && action == "EnableValve") {
        uint8_t port = item.toInt();
        if (item && port > 0) {
          if (newState) VStruct->SetEnable(port, true);
          if (!newState) VStruct->SetEnable(port, false);
          json["response"]["status"] = 1;
          json["response"]["text"] = (VStruct->GetEnabled(port)?"valve now enabled":"valve now disabled");
        }
      }

      #ifdef USE_I2C
      if (action && action == "RefreshI2C") {
        I2Cdetect->i2cScan();  
        
        json["data"].to<JsonObject>();
        json["data"]["showI2C"] = I2Cdetect->i2cGetAddresses();
        json["response"]["status"] = 1;
        json["response"]["text"] = "successful";
      }
      #endif
      
      #ifdef USE_ONEWIRE
      if (action && action == "Refresh1Wire") {
        uint8_t ow = VStruct->Refresh1WireDevices();  
        
        String buffer = String(ow) + " (" + String(ow * 8) + ")";
        
        json["data"].to<JsonObject>();
        json["data"]["show1Wire"] = buffer;
        json["response"]["status"] = 1;
        json["response"]["text"] = "successful";
      }
      #endif

    } else {
      Config->logN(1, "WebSocket data received but not a valid json string: %s -> %s", msg.c_str(), error.c_str());
      json["response"]["status"] = 0;
      json["response"]["text"] = error.c_str();
    }

    ws->text(client->id(), json.as<String>());

  }
}

void MyWebServer::GetInitDataNavi(JsonDocument& json){
  json["data"].to<JsonObject>();
  json["data"]["hostname"] = Config->GetMqttRoot();
  json["data"]["releasename"] = Config->GetReleaseName();
  json["data"]["releasedate"] = __DATE__;
  json["data"]["releasetime"] = __TIME__;

  #ifdef USE_ONEWIRE
    if (VStruct->Get1WireCountDevices()==0) { 
      json["data"]["td_1wire_0"]["className"] = "hide"; 
      json["data"]["1wireconfig"]["className"] = "hide";
    }
  #else
    json["data"]["td_1wire_0"]["className"] = "hide";
    json["data"]["1wireconfig"]["className"] = "hide";
  #endif

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

void MyWebServer::GetInitDataStatus(JsonDocument& json) {
  json["data"].to<JsonObject>();
  json["data"]["ipaddress"] = mqtt->GetIPAddress().toString();
  json["data"]["wifiname"] = (Config->GetUseETH()?"LAN":WiFi.SSID());
  json["data"]["macaddress"] = WiFi.macAddress();
  json["data"]["bssid"] = (Config->GetUseETH()?"wired LAN":WiFi.BSSIDstr());
  json["data"]["mqtt_status"] = (mqtt->GetConnectStatusMqtt()?"Connected":"Not Connected");
  json["data"]["uptime"] = uptime_formatter::getUptime();
  json["data"]["freeheapmem"] = ESP.getFreeHeap();
  json["data"]["ValvesCount"] = VStruct->CountActiveThreads();
  json["data"]["fragmentation"] = Config->getFragmentation();
  
  #ifdef USE_I2C
    json["data"]["showI2C"] = I2Cdetect->i2cGetAddresses();
  #else
    json["data"]["tr_i2c"]["className"] = "hide";
  #endif

  #ifdef USE_ONEWIRE
    if (Config->Enabled1Wire()) {
      json["data"]["show1Wire"] = VStruct->Get1WireCountDevices()*8;
    } else { 
      json["data"]["tr_1wire"]["className"] = "hide";
    }
  #else
    json["data"]["tr_1wire"]["className"] = "hide";
  #endif

  if (LevelSensor->GetType() != NONE && LevelSensor->GetType() != EXTERN) { 
    json["data"]["SensorRawValue"] = LevelSensor->GetRaw();
  } else {
    json["data"]["tr_sensRaw"]["className"] = "hide";
  }

  if (LevelSensor->GetType() != NONE) {
    json["data"]["SensorLevel"] = LevelSensor->GetLvl();
  } else {
    json["data"]["tr_sensLvl"]["className"] = "hide";
  }

  #ifdef ESP32
    String rssi = (String)(Config->GetUseETH()?ETH.linkSpeed():WiFi.RSSI());
    if (Config->GetUseETH()) rssi.concat(" Mbps");  
    json["data"]["rssi"] = rssi;
  #else
    json["data"]["rssi"] = WiFi.RSSI();
  #endif

  #ifndef USE_WEBSERIAL
    json["data"]["tr_webserial"]["className"] = "hide";
  #endif

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

void MyWebServer::flowerCareOnMqttMessage(String& topic, String& JsonMsg) {
  Config->logN(4, "FlowerCare MQTT Message received: %s", JsonMsg.c_str());
  // lade JsonMsg als JsonDocument
  JsonDocument json;
  DeserializationError error = deserializeJson(json, JsonMsg.c_str());
  if (error) {
    Config->logN(1, "Failed to parse MQTT message: %s", error.c_str());
    return;
  }

  if (json["address"]) {
    if (json["moisture"].as<int>() == 0) {
      Config->logN(4, "FlowerCare %s: moisture: %d%% -> do nothing because moisture value is 0\n",
        json["address"].as<String>().c_str(),
        json["moisture"].as<int>());
      return;
    }
    for (uint8_t i = 0; i < _relationen->size(); i++) {
      if (_relationen->at(i).TriggerTopic == topic && _relationen->at(i).enabled) {
        if (json["moisture"].as<int>() < _relationen->at(i).threshold) {
          VStruct->OnForTimer(_relationen->at(i).ActorPort, _relationen->at(i).duration);
          Config->logN(3, "FlowerCare %s (moisture: %d%%) triggered valve %d for %d seconds", 
              json["address"].as<String>(),
              json["moisture"].as<int>(),
              _relationen->at(i).ActorPort,
              _relationen->at(i).duration);
        }
      }
    }
  }
}

void MyWebServer::GetInitDataFlowerCare(JsonDocument& json) {
#ifdef USE_FLOWERCARE  
  const std::vector<FlowerCareDevice>* devices = flowerCare->getDevices();
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
#endif

  JsonArray f = json["data"]["fc_relations"].to<JsonArray>();
  for (uint8_t i = 0; i < _relationen->size(); i++) {
    JsonObject o = f.add<JsonObject>();
    o["active"]["checked"] = _relationen->at(i).enabled;
    o["mqtttopic"] = _relationen->at(i).TriggerTopic;
    o["ConfiguredPort"] = _relationen->at(i).ActorPort;
    o["threshold"] = _relationen->at(i).threshold;
    o["duration"] = _relationen->at(i).duration;
  }
  
  json["js"]["esp_uptime"] = millis();
  VStruct->getWebJsParameter(json); // add JSParameter

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

void MyWebServer::LoadFlowerCareConfig() {
  _relationen->clear(); // leere den Vector bevor neu befuellt wird
  mqtt->ClearSubscriptions(MyMQTT::FLOWERCARE);
 
  bool loadDefaultConfig = false;
 
  if (LittleFS.exists("/config/flowercare.json")) {
    //file exists, reading and loading
    Config->logN(3, "reading flowercare.json file....");
    File configFile = LittleFS.open("/config/flowercare.json", "r");
    if (configFile) {
      Config->logN(3, "flowercare.json is now open");
 
      ReadBufferingStream stream{configFile, 64};
      stream.find("\"data\":[");
      do {
        JsonDocument elem;
        DeserializationError error = deserializeJson(elem, stream); 
 
        if (error) {
           loadDefaultConfig = true;
           Config->logN(1, "Failed to parse flowercare.json data: %s, load default config", error.c_str()); 
        } else {
           // Print the result
           Config->logN(3, "parsing JSON ok");
           Config->log(4, elem);
 
          if (elem["mqtttopic"] && elem["port"] && elem["port"] && elem["port"].as<int>() > 0) {
            FlowercareRelation_t rel;
 
            rel.enabled = elem["active"].as<bool>();
            rel.TriggerTopic = elem["mqtttopic"].as<String>();
            rel.threshold = elem["threshold"].as<unsigned int>();
            rel.duration = elem["duration"].as<unsigned int>();
            rel.ActorPort = elem["port"].as<uint8_t>();

            _relationen->push_back(rel);
            mqtt->Subscribe(rel.TriggerTopic, MyMQTT::FLOWERCARE);
          } 
          
          #ifdef USE_FLOWERCARE
          if (elem["address"]) {
            // activation of known Flowercare devices
            flowerCare->addDevice(NimBLEAddress(elem["address"].as<String>().c_str(), BLE_ADDR_PUBLIC));
            flowerCare->setActive(elem["address"].as<String>(), elem["active"].as<bool>());
          }
          #endif
        }
      } while (stream.findUntil(",","]"));
    } else {
      loadDefaultConfig = true;
      Config->logN(1, "failed to load flowercare.json, load default config");
    }
  } else {
    loadDefaultConfig = true;
    Config->logN(3, "flowercare.json File not exists, load default config");
  }
   
  if (loadDefaultConfig) {
    Config->logN(3, "load flowercare DefaultConfig");
    FlowercareRelation_t rel;
    rel.enabled = false;
    rel.TriggerTopic = "flowercare/00:00:00:00:00:00";
    rel.threshold = 30;
    rel.duration = 60;
    rel.ActorPort = 0;
     _relationen->push_back(rel);
  }
  Config->logN(3, "%d flowercare relations are now loaded ", _relationen->size());
 
  _relationen->shrink_to_fit();
 }

#ifdef USE_FLOWERCARE
void MyWebServer::flowerCareGetValuesCallback(JsonDocument& json) {
  // sending over MQTT
  json["host"] = Config->GetMqttRoot();
  String topic = "flowercare/" + json["address"].as<String>();
  Config->logN(4, "Sending FlowerCare data to MQTT: %s -> %s", topic.c_str(), json.as<String>().c_str());
  mqtt->Publish_String(topic.c_str(), json.as<String>(), true);

  // sending over WebSocket, has to reformat the json
  JsonDocument wsjson;
  wsjson["data-id"][String(json["address"].as<String>()) + "_temp"] = json["temperature"];
  wsjson["data-id"][String(json["address"].as<String>()) + "_moist"] = json["moisture"];
  wsjson["data-id"][String(json["address"].as<String>()) + "_bright"] = json["brightness"];
  wsjson["data-id"][String(json["address"].as<String>()) + "_fert"] = json["fertility"];
  wsjson["data-id"][String(json["address"].as<String>()) + "_liveupd"] = millis()-2000;  //updatetime 2sec ago
  
  if (json["battery"]) {
    wsjson["data-id"][String(json["address"].as<String>()) + "_bat"] = json["battery"];
    wsjson["data-id"][String(json["address"].as<String>()) + "_fw"] = json["firmwareVersion"];
    wsjson["data-id"][String(json["address"].as<String>()) + "_batupd"] = millis()-2000;  // updatetime 2sec ago
  } 

  wsjson["js"]["esp_uptime"] = millis();
  wsjson["cmd"]["callbackFn"] = "onBleUpdate_Callback";
  wsjson["cmd"]["highlight"] = "true";

  ws->textAll(wsjson.as<String>());
}

void MyWebServer::flowerCareOnScanEndCallback() {
  Config->logN(3, "FlowerCare scan ended");
  JsonDocument json;
  this->GetInitDataFlowerCare(json);

  json["response"]["status"] = 1;
  json["response"]["text"] = "scan ended";
  
  ws->textAll(json.as<String>());
}
#endif