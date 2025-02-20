#include "mywebserver.h" 

MyWebServer::MyWebServer(AsyncWebServer *server, DNSServer* dns): 
        server(server), 
        dns(dns), 
        DoReboot(false),
        RequestRebootTime(0) {
  
  fsfiles = new handleFiles(server);
  //fsfiles->registerLogCallback([this](int loglevel, const char* format, va_list args) {
  //  Config->logN(loglevel, format, args);
  //});

  fsfiles->registerLogCallback(std::bind(&BaseConfig::logN, Config, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  ws = new AsyncWebSocket("/ajaxws");

  ElegantOTA.begin(server);
  ElegantOTA.setGitEnv(String(GIT_OWNER), String(GIT_REPO), String(GIT_BRANCH), String(GITHUB_RUN).toInt());
  ElegantOTA.setFWVersion(String(Config->GetReleaseName() + " / Build: " + GITHUB_RUN ));
  ElegantOTA.setBackupRestoreFS("/config");
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
  
  server->serveStatic("/", LittleFS, "/", "max-age=3600").setDefaultFile("/web/index.html");

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
}

void MyWebServer::handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

bool MyWebServer::handleReset() {
  bool ret = true;
  Config->logN(3, "deletion of all config files was requested ....");
  //LittleFS.format(); // Werkszustand -> nur die config dateien loeschen, die register dateien muessen erhalten bleiben
  File root = LittleFS.open("/config/");
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
    Config->logN(2, "[Client: %u] WebSocket client connected", client->id());
  
  } else if (type == WS_EVT_DISCONNECT) {
    Config->logN(2, "[Client: %u] WebSocket client disconnected", client->id());
  
  } else if (type == WS_EVT_DATA) {
    String msg(""); msg.reserve(len + 1);
    for (size_t i = 0; i < len; i++) { msg += (char)data[i]; } msg += '\0';
    Config->logN(2, "[Client: %u] WebSocket data received: %s", client->id(), msg.c_str()); 

    String action(""), subaction(""), item("");
    bool newState = false;
    uint8_t port = 0;
    JsonDocument json;
    DeserializationError error = deserializeJson(json, msg.c_str());
    if (!error) {
      if (json["cmd"]) {
        if (json["cmd"]["action"])      { action    = json["cmd"]["action"].as<String>();}
        if (json["cmd"]["subaction"])   { subaction = json["cmd"]["subaction"].as<String>();}
        if (json["cmd"]["newState"])    { newState  = json["cmd"]["newState"].as<bool>();}
        if (json["cmd"]["port"])        { port = json["cmd"]["port"].as<int>(); }
        
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
        }
      }

      if(action && action == "ReloadConfig")  {
        if (subaction && subaction == "baseconfig") {
          Config->LoadJsonConfig();
        } else if (subaction && subaction == "valveconfig") {
          VStruct->LoadJsonConfig();
        } else if (subaction && subaction == "sensorconfig") {
          LevelSensor->LoadJsonConfig();
        } else if (subaction && subaction == "relations") {
          ValveRel->LoadJsonConfig();
        }
      
        json["response"]["status"] = 1;
        json["response"]["text"] = "new config reloaded sucessfully";
      }

      if(action && action == "handlefiles") {
        fsfiles->HandleRequest(json);
      }

      if(action && action == "SetValve") {
        if (newState && port && port > 0 && !VStruct->GetEnabled(port)) { 
          json["response"]["status"] = 0; 
          json["response"]["text"] = "Requested Port not enabled. Please enable first!";
        }
        else if (port && port > 0 )  { 
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
        if (port && port > 0) {
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

