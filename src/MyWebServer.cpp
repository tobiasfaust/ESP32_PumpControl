#include "MyWebServer.h" 

MyWebServer::MyWebServer(AsyncWebServer *server, DNSServer* dns): server(server), dns(dns), DoReboot(false) {
  
  fsfiles = new handleFiles(server);

  ElegantOTA.begin(server);
  ElegantOTA.onStart(std::bind(&MyWebServer::onOTAStart, this));
  ElegantOTA.onProgress(std::bind(&MyWebServer::onOTAProgress, this, std::placeholders::_1, std::placeholders::_2));
  ElegantOTA.onEnd(std::bind(&MyWebServer::onOTAEnd, this, std::placeholders::_1));
  
  server->begin();

  server->onNotFound(std::bind(&MyWebServer::handleNotFound, this, std::placeholders::_1));
  server->on("/reboot",       HTTP_GET, std::bind(&MyWebServer::handleReboot, this, std::placeholders::_1));
  server->on("/reset",        HTTP_GET, std::bind(&MyWebServer::handleReset, this, std::placeholders::_1));
  server->on("/wifireset",    HTTP_GET, std::bind(&MyWebServer::handleWiFiReset, this, std::placeholders::_1));
  server->on("/parameter.js", HTTP_GET, std::bind(&MyWebServer::handleJSParam, this, std::placeholders::_1));
  server->on("/ajax",         HTTP_POST,std::bind(&MyWebServer::handleAjax, this, std::placeholders::_1));
  
  server->serveStatic("/", LittleFS, "/", "max-age=3600").setDefaultFile("/web/index.html");

  dbg.println(F("WebServer started..."));
}


void MyWebServer::onOTAStart() {
  // Log when OTA has started
  dbg.println("OTA update started!");
}

void MyWebServer::onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    dbg.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void MyWebServer::onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    dbg.println("OTA update finished successfully!");
  } else {
    dbg.println("There was an error during OTA update!");
  }
}

void MyWebServer::loop() {
  //delay(1); // slow response Issue: https://github.com/espressif/arduino-esp32/issues/4348#issuecomment-695115885
  if (this->DoReboot) {
    dbg.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

  ElegantOTA.loop();
}

void MyWebServer::handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void MyWebServer::handleReboot(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/web/reboot.html", "text/html");
  this->DoReboot = true;
}

void MyWebServer::handleReset(AsyncWebServerRequest *request) {
  if (Config->GetDebugLevel() >= 3) { dbg.println("deletion of all config files was requested ...."); }
  //LittleFS.format(); // Werkszustand -> nur die config dateien loeschen, die web dateien muessen erhalten bleiben
  File root = LittleFS.open("/", "r");
  File file = root.openNextFile();
  while(file){
    String path("/"); path.concat(file.name());
    if (path.indexOf(".json") == -1) {dbg.println("Continue"); file = root.openNextFile(); continue;}
    file.close();
    bool rm = LittleFS.remove(path);
    if (Config->GetDebugLevel() >= 3) {
      dbg.printf("deletion of configuration file '%s' %s\n", file.name(), (rm?"was successful":"has failed"));;
    }
    file = root.openNextFile();
  }
  root.close();

  this->handleReboot(request);
}

void MyWebServer::handleWiFiReset(AsyncWebServerRequest *request) {
  #ifdef ESP32
    WiFi.disconnect(true,true);
  #elif ESP8266  
    ESP.eraseConfig();
  #endif
  
  this->handleReboot(request);
}

void MyWebServer::handleJSParam(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("text/javascript");
  response->addHeader("Server","ESP Async Web Server");

  VStruct->getWebJsParameter(response);
  request->send(response);
}

void MyWebServer::handleAjax(AsyncWebServerRequest *request) {
  char buffer[100] = {0};
  memset(buffer, 0, sizeof(buffer));
  String ret = (char*)0;
  bool RaiseError = false;
  String action, subaction, newState; 
  String json = "{}";
  uint8_t port = 0;

  AsyncResponseStream *response = request->beginResponseStream("text/json");
  response->addHeader("Server","ESP Async Web Server");

  if(request->hasArg("json")) {
    json = request->arg("json");
  }

  JsonDocument jsonGet; 
  DeserializationError error = deserializeJson(jsonGet, json.c_str());

  JsonDocument jsonReturn;
  jsonReturn["response"].to<JsonObject>();

  if (Config->GetDebugLevel() >=4) { dbg.print("Ajax Json Empfangen: "); }
  if (!error) {
    if (Config->GetDebugLevel() >=4) { serializeJsonPretty(jsonGet, dbg); dbg.println(); }

    if (jsonGet.containsKey("action"))   {action    = jsonGet["action"].as<String>();}
    if (jsonGet.containsKey("subaction")){subaction = jsonGet["subaction"].as<String>();}
    if (jsonGet.containsKey("newState")) { newState = jsonGet["newState"].as<String>(); }
    if (jsonGet.containsKey("port"))     { port = jsonGet["port"].as<int>(); }
  
  } else { 
    snprintf(buffer, sizeof(buffer), "Ajax Json Command not parseable: %s -> %s", json.c_str(), error.c_str());
    RaiseError = true; 
  }

  if (RaiseError) {
    jsonReturn["response"]["status"] = 0;
    jsonReturn["response"]["text"] = buffer;
    serializeJson(jsonReturn, ret);
    response->print(ret);

    if (Config->GetDebugLevel() >=2) {
      dbg.println(FPSTR(buffer));
    }

    return;
    
  } else if(action && action == "GetInitData")  {
    if (subaction && subaction == "status") {
      this->GetInitDataStatus(response);
    } else if (subaction && subaction == "navi") {
      this->GetInitDataNavi(response);
    } else if (subaction && subaction == "baseconfig") {
      Config->GetInitData(response);
    } else if (subaction && subaction == "valveconfig") {
      VStruct->GetInitData(response);
    } else if (subaction && subaction == "1wireconfig") {
      VStruct->GetInitData1Wire(response);
    }else if (subaction && subaction == "sensorconfig") {
      LevelSensor->GetInitData(response);
    } else if (subaction && subaction == "relations") {
      ValveRel->GetInitData(response);
    }
  
  } else if(action && action == "ReloadConfig")  {
    if (subaction && subaction == "baseconfig") {
      Config->LoadJsonConfig();
    } else if (subaction && subaction == "valveconfig") {
      VStruct->LoadJsonConfig();
    } else if (subaction && subaction == "sensorconfig") {
      LevelSensor->LoadJsonConfig();
    } else if (subaction && subaction == "relations") {
      ValveRel->LoadJsonConfig();
    }
  
    jsonReturn["response"]["status"] = 1;
    jsonReturn["response"]["text"] = "new config reloaded sucessfully";
    serializeJson(jsonReturn, ret); 
    response->print(ret);
  
  } else if(action && action == "handlefiles") {
    fsfiles->HandleAjaxRequest(jsonGet, response);

  } else if (action && action == "SetValve") {
      if (newState && port && port > 0 && !VStruct->GetEnabled(port)) { 
        jsonReturn["response"]["status"] = 0; 
        jsonReturn["response"]["text"] = "Requested Port not enabled. Please enable first!";
        serializeJson(jsonReturn, ret);
        response->print(ret);
      }
      else if (newState && port && port > 0 )  { 
        if (newState == "On") {
          VStruct->SetOn(port); 
        }
        if (newState == "Off") { 
          VStruct->SetOff(port); 
        }

        jsonReturn["response"]["status"] = 1;
        jsonReturn["response"]["text"] =(VStruct->GetState(port)?"Valve is now: ON":"Valve is now: OFF");
        jsonReturn["data"][subaction] = (VStruct->GetState(port)?"Set Off":"Set On"); // subaction = button.id
        serializeJson(jsonReturn, ret);
        response->print(ret);
      }

  
  } else if (action && newState && action == "EnableValve") {
      if (port && port > 0 && newState) {
        if (strcmp(newState.c_str(),"true")==0) VStruct->SetEnable(port, true);
        if (strcmp(newState.c_str(),"false")==0) VStruct->SetEnable(port, false);
        jsonReturn["response"]["status"] = 1;
        jsonReturn["response"]["text"] = (VStruct->GetEnabled(port)?"valve now enabled":"valve now disabled");
        serializeJson(jsonReturn, ret);
        response->print(ret);
      }
  
#ifdef USE_I2C
  } else if (action && action == "RefreshI2C") {
      I2Cdetect->i2cScan();  
      
      jsonReturn["data"].to<JsonObject>();
      jsonReturn["data"]["showI2C"] = I2Cdetect->i2cGetAddresses();
      jsonReturn["response"]["status"] = 1;
      jsonReturn["response"]["text"] = "successful";
      serializeJson(jsonReturn, ret);
      response->print(ret);
#endif

#ifdef USE_ONEWIRE
  } else if (action && action == "Refresh1Wire") {
      uint8_t ow = VStruct->Refresh1WireDevices();  
      snprintf(buffer, sizeof(buffer), "%d (%d)", ow, ow * 8);
      
      jsonReturn["data"].to<JsonObject>();
      jsonReturn["data"]["show1Wire"] = buffer;
      jsonReturn["response"]["status"] = 1;
      jsonReturn["response"]["text"] = "successful";
      serializeJson(jsonReturn, ret);
      response->print(ret);
  #endif

  } else {
    snprintf(buffer, sizeof(buffer), "Ajax Command unknown: %s - %s", action.c_str(), subaction.c_str());
    jsonReturn["response"]["status"] = 0;
    jsonReturn["response"]["text"] = buffer;
    serializeJson(jsonReturn, ret);
    response->print(ret);

    if (Config->GetDebugLevel() >=1) {
      dbg.println(buffer);
    }
  }
  
  if (Config->GetDebugLevel() >=4) { dbg.print("Ajax Json Antwort: "); dbg.println(ret); }
  
  request->send(response);
}

void MyWebServer::GetInitDataNavi(AsyncResponseStream *response){
  String ret;
  JsonDocument json;
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
  serializeJson(json, ret);
  response->print(ret);
}

void MyWebServer::GetInitDataStatus(AsyncResponseStream *response) {
  String ret;
  JsonDocument json;
  
  json["data"].to<JsonObject>();
  json["data"]["ipaddress"] = mqtt->GetIPAddress().toString();
  json["data"]["wifiname"] = (Config->GetUseETH()?"LAN":WiFi.SSID());
  json["data"]["macaddress"] = WiFi.macAddress();
  json["data"]["mqtt_status"] = (mqtt->GetConnectStatusMqtt()?"Connected":"Not Connected");
  json["data"]["uptime"] = uptime_formatter::getUptime();
  json["data"]["freeheapmem"] = ESP.getFreeHeap();
  json["data"]["ValvesCount"] = VStruct->CountActiveThreads();
  
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
    json["data"]["rssi"] = (Config->GetUseETH()?ETH.linkSpeed():WiFi.RSSI()), (Config->GetUseETH()?"Mbps":"");
  #else
    json["data"]["rssi"] = WiFi.RSSI();
  #endif

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";

  serializeJson(json, ret);
  response->print(ret);
}

