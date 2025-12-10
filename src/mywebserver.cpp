#include "mywebserver.h" 

MyWebServer::MyWebServer(fs::LittleFSFS& sysFS, fs::LittleFSFS& configFS, AsyncWebServer *server, DNSServer* dns): 
        sysFS(sysFS),
        configFS(configFS),
        server(server), 
        dns(dns), 
        DoReboot(false),
        RequestRebootTime(0) {
  
  fsfiles = new handleFiles(server);
  fsfiles->registerLogCallback(std::bind(&BaseConfig::logN, Config, std::placeholders::_1, std::placeholders::_2));
  fsfiles->registerLittleFS(&sysFS, "/web");
  fsfiles->registerLittleFS(&configFS, "/config");

  _wsclientRequests = new std::vector<wsclient_t>();
  
  Config->logN(1, "Starting DoIf Module");
  doif = new doIf(configFS);

  #ifdef USE_FLOWERCARE
    Config->logN(1, "Starting FlowerCare");
    flowerCare = new flowercareWeb(configFS);
    flowerCare->onLog(std::bind(&BaseConfig::logN, Config, std::placeholders::_1, std::placeholders::_2));
    flowerCare->onScanEnd(std::bind(&MyWebServer::flowerCareOnScanEndCallback, this));
    flowerCare->LoadJsonConfig();
  #endif

  this->ws = new AsyncWebSocket("/ajaxws");

  ElegantOTA.setTargetPartition("webdata");  // Set default partition for OTA updates
  ElegantOTA.setGitEnv(String(GIT_OWNER), String(GIT_REPO), String(GIT_BRANCH), String(GITHUB_RUN).toInt());
  ElegantOTA.setFWVersion(String(Config->GetReleaseName() + " / Build: " + GITHUB_RUN ));
  ElegantOTA.setFWVariant(String(GIT_VARIANT));
  ElegantOTA.setAutoReboot(true);
  ElegantOTA.onStart(std::bind(&MyWebServer::onOTAStart, this));
  ElegantOTA.onProgress(std::bind(&MyWebServer::onOTAProgress, this, std::placeholders::_1, std::placeholders::_2));
  ElegantOTA.onEnd(std::bind(&MyWebServer::onOTAEnd, this, std::placeholders::_1));
  ElegantOTA.begin(server);
  

  server->on("/", HTTP_GET, std::bind(&MyWebServer::handleRoot, this, std::placeholders::_1), nullptr, nullptr);
  server->onNotFound(std::bind(&MyWebServer::handleNotFound, this, std::placeholders::_1));
  
  this->ws->onEvent(std::bind(&MyWebServer::onWsEvent, this, std::placeholders::_1, 
    std::placeholders::_2, 
    std::placeholders::_3, 
    std::placeholders::_4, 
    std::placeholders::_5, 
    std::placeholders::_6 ));

  server->addHandler(ws);
  
  server->serveStatic("/web/", sysFS, "/", "max-age=3600").setDefaultFile("/web/index.html");
  server->serveStatic("/config/", configFS, "/");

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
  this->ws->cleanupClients();

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
  Config->logN(3, "deletion of all config files was requested ....");

  bool result = configFS.format();
  if (!result) {
    Config->logN(2, "formatting of config Filesystem failed");
  } else {
    Config->logN(4, "formatting of config Filesystem was successful");
  }
  this->DoReboot = true;

  return result;
}

void MyWebServer::onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Config->logN(4, "[Client: %u] WebSocket client connected", client->id());
  
  } else if (type == WS_EVT_DISCONNECT) {
    Config->logN(4, "[Client: %u] WebSocket client disconnected", client->id());

    // Remove client from WebSocket client requests if it exists
    for (uint8_t i = 0; i < this->_wsclientRequests->size(); i++) {
      if (this->_wsclientRequests->at(i).ws_id == client->id()) {
        
        #ifdef USE_FLOWERCARE
        if (this->_wsclientRequests->at(i).requestData == wsclient_t::FLOWERCARE_DATA) { flowerCare->onValues(nullptr); }
        #endif

        if (this->_wsclientRequests->at(i).requestData == wsclient_t::ADS1115_DATA) { LevelSensor->onValues(nullptr); }
        if (this->_wsclientRequests->at(i).requestData == wsclient_t::FLOWCONTROL_DATA) { FlowCtrl->onValues(nullptr); }
        if (this->_wsclientRequests->at(i).requestData == wsclient_t::LOG_DATA) { Config->onLogValues(nullptr); }

        this->_wsclientRequests->erase(this->_wsclientRequests->begin() + i);
      }
    }
    _wsclientRequests->shrink_to_fit();

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

      if (action && action == "subscribe") {
        if (subaction && subaction == "flowercare_data") {
          #ifdef USE_FLOWERCARE
            this->_wsclientRequests->push_back({client->id(), wsclient_t::FLOWERCARE_DATA});
            flowerCare->onValues(std::bind(&MyWebServer::flowerCareGetValuesCallback, this, std::placeholders::_1, client->id()));
          #endif
        } else if (subaction && subaction == "ads1115_data") {
          this->_wsclientRequests->push_back({client->id(), wsclient_t::ADS1115_DATA});
          LevelSensor->onValues(std::bind(&MyWebServer::LevelSensorGetValuesCallback, this, std::placeholders::_1, client->id()));
        } else if (subaction && subaction == "flowcontrol_data") {
          this->_wsclientRequests->push_back({client->id(), wsclient_t::FLOWCONTROL_DATA});
          FlowCtrl->onValues(std::bind(&MyWebServer::flowControlGetValuesCallback, this, std::placeholders::_1, client->id()));
        } else if (subaction && subaction == "log_data") {
          this->_wsclientRequests->push_back({client->id(), wsclient_t::LOG_DATA});
          Config->onLogValues(std::bind(&MyWebServer::logGetValuesCallback, this, std::placeholders::_1, json, client->id()));
        }
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
          json["js"]["gpio_disabled"] = Config->disabledGPIO.getArrayExcludeIdentifier(BaseConfig::GpioIdentifier::BASECONFIG);
        } else if (subaction && subaction == "valveconfig") {
          VStruct->GetInitData(json);
          VStruct->getWebJsParameter(json);
          json["js"]["gpio_disabled"] = Config->disabledGPIO.getArrayExcludeIdentifier(BaseConfig::GpioIdentifier::VALVES);
        } else if (subaction && subaction == "1wireconfig") {
          VStruct->GetInitData1Wire(json);
          //VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "sensorconfig") {
          LevelSensor->GetInitData(json);
          //VStruct->getWebJsParameter(json);
          json["js"]["gpio_disabled"] = Config->disabledGPIO.getArrayExcludeIdentifier(BaseConfig::GpioIdentifier::SENSOR);
        } else if (subaction && subaction == "relations") {
          ValveRel->GetInitData(json);
          VStruct->getWebJsParameter(json);
        } else if (subaction && subaction == "flowcontrol") {
          FlowCtrl->GetInitData(json);
          VStruct->getWebJsParameter(json);
          json["js"]["gpio_disabled"] = Config->disabledGPIO.getArrayExcludeIdentifier(BaseConfig::GpioIdentifier::FLOWCONTROL);
        } else if (subaction && subaction == "flowercare") {
          #ifdef USE_FLOWERCARE
            flowerCare->GetInitData(json);
            VStruct->getWebJsParameter(json);
          #endif
        } else if (subaction && subaction == "doif") {
          doif->GetInitData(json);
          VStruct->getWebJsParameter(json);
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

        if (subaction && subaction == "flowcontrol") {
          FlowCtrl->LoadJsonConfig();
        }

        #ifdef USE_FLOWERCARE
          if (subaction && subaction == "flowercare") {
            flowerCare->LoadJsonConfig();
          }
        #endif

        if (subaction && subaction == "doif") {
          doif->LoadJsonConfig();
        }
      
        json["response"]["status"] = 1;
        json["response"]["text"] = "new config reloaded sucessfully";
      }

      if(action && action == "handlefiles") {
        fsfiles->HandleRequest(json);
      }
     
      #ifdef USE_FLOWERCARE
      if (action && action == "flowercare") {
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
      }
      #endif

      if (action && action == "doif") {
        if (subaction && subaction == "activateRelation") {
          if (item && item2 && newState) {
            // item -> mqtttopic, item2 -> port
            if (doif->setActive(item, item2.toInt(), newState)) {
              if (newState) mqtt->Subscribe(item, MyMQTT::DOIF); // do not unsubscribe, because it is possible that the same topic is used by another relation
              json["response"]["status"] = 1;
              json["response"]["text"] = (newState ? "relation activated" : "relation deactivated");
            } else {
              json["response"]["status"] = 0;
              json["response"]["text"] = "relation not found, please save first";
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

    this->ws->text(client->id(), json.as<String>());

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

  #ifndef USE_FLOWERCARE
    json["data"]["td_flowercare"]["className"] = "hide"; 
    json["data"]["flowercare"]["className"] = "hide";
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

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

void MyWebServer::DoIfOnMqttMessage(String& topic, String& msg) {
  doif->onMqttMessage(VStruct, topic, msg);
}

#ifdef USE_FLOWERCARE
void MyWebServer::flowerCareGetValuesCallback(JsonDocument& json, uint32_t wsclient_id) {
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

  this->ws->text(wsclient_id, wsjson.as<String>());
}

void MyWebServer::flowerCareOnScanEndCallback() {
  Config->logN(3, "FlowerCare scan ended");
  JsonDocument json;
  flowerCare->GetInitData(json);

  json["response"]["status"] = 1;
  json["response"]["text"] = "scan ended";
  
  this->ws->textAll(json.as<String>());
}
#endif

void MyWebServer::flowControlGetValuesCallback(JsonDocument& json, uint32_t wsclient_id) {
  // sending over WebSocket, has to reformat the json
  JsonDocument wsjson;
  JsonArray rows = json.as<JsonArray>();
  for (JsonObject elem : rows) {
    wsjson["data-id"][String(elem["name"].as<String>()) + "_l/min"] = String(elem["l/min"].as<float>(), 2);
    wsjson["data-id"][String(elem["name"].as<String>()) + "_total"] = String(elem["total"].as<float>(), 2);
  }
  wsjson["cmd"]["highlight"] = "true";
  this->ws->text(wsclient_id, wsjson.as<String>());
}

void MyWebServer::LevelSensorGetValuesCallback(JsonDocument& json, uint32_t wsclient_id) {
  // sending over WebSocket, has to reformat the json
  JsonDocument wsjson;
  JsonArray rows = json.as<JsonArray>();
  for (JsonObject elem : rows) {
    wsjson["data-id"][String(elem["name"].as<String>()) + "_val"] = String(elem["moisture"].as<String>() + "%");
  }
  wsjson["cmd"]["highlight"] = "true";
  this->ws->text(wsclient_id, wsjson.as<String>());
}

void MyWebServer::logGetValuesCallback(const char* logline, JsonDocument& json, uint32_t wsclient_id) {
  json["logline"] = logline;
  this->ws->text(wsclient_id, json.as<String>());
}
