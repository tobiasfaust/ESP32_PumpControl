#include "baseconfig.h"

BaseConfig::BaseConfig(fs::LittleFSFS& configFS) :
  configFS(configFS), 
  mqtt_server ("test.mosquitto.org"),
  mqtt_port(1883),
  mqtt_root("PumpControl"),
  mqtt_basepath("home/"),
  mqtt_UseRandomClientID(true),
  keepalive(0),
  debuglevel(3),
  pin_1wire(0),
  enable_oled(false),
  oled_type(0),
  enable_1wire(false),
  i2caddress_oled(60), //0x3C;
  enable_3wege(false),
  ventil3wege_port(0),
  max_parallel(0),
  useETH(0),
  serial_rx(3),
  serial_tx(1)
  {
  
  #ifdef ESP8266
    this->pin_sda = 5;
    this->pin_scl = 4;
  #endif
  
  #ifdef ESP32
    this->pin_sda = 21;
    this->pin_scl = 22,
  #endif
  
  LoadJsonConfig();
}

void BaseConfig::LoadJsonConfig() {
  if (configFS.exists("/baseconfig.json")) {
    //file exists, reading and loading
    this->logN(3, "reading baseconfig.json file");
    File configFile = configFS.open("/baseconfig.json", "r");
    if (configFile) {
      this->logN(3, "baseconfig.json is now open");
      ReadBufferingStream stream{configFile, 64};
      stream.find("\"data\":[");
      do {

        JsonDocument elem;
        DeserializationError error = deserializeJson(elem, stream); 
        if (error) {
          this->logN(1, "Failed to parse baseconfig.json data: %s, load default config", error.c_str()); 
        } else {
          // Print the result
          this->logN(5, "parsing partial JSON of baseconfig.json ok"); 
          this->log(5, elem);
          
          if (elem["SelectConnectivity"]){ this->useETH = (elem["SelectConnectivity"].as<String>()=="eth"?1:0); }
          if (elem["SelectLAN"])        { this->LANBoard = elem["SelectLAN"].as<String>(); }  
          if (elem["mqttroot"])         { this->mqtt_root = elem["mqttroot"].as<String>();}
          if (elem["mqttserver"])       { this->mqtt_server = elem["mqttserver"].as<String>();}
          if (elem["mqttport"])         { this->mqtt_port = elem["mqttport"].as<int>();}
          if (elem["mqttuser"])         { this->mqtt_username = elem["mqttuser"].as<String>();}
          if (elem["mqttpass"])         { this->mqtt_password = elem["mqttpass"].as<String>();}
          if (elem["mqttbasepath"])     { this->mqtt_basepath = elem["mqttbasepath"].as<String>();}
          if (elem["sel_UseRandomClientID"]){ if (strcmp(elem["sel_UseRandomClientID"], "none")==0) { this->mqtt_UseRandomClientID=false;} else {this->mqtt_UseRandomClientID=true;}}
          if (elem["keepalive"])        { if (elem["keepalive"].as<int>() == 0) { this->keepalive = 0;} else { this->keepalive = _max(elem["keepalive"].as<int>(), 10);}}
          if (elem["debuglevel"])       { this->debuglevel = _max(elem["debuglevel"].as<int>(), 0);}
          if (elem["pinsda"])           { this->pin_sda = (elem["pinsda"].as<int>()) - 200;}
          if (elem["pinscl"])           { this->pin_scl = (elem["pinscl"].as<int>()) - 200;}
          if (elem["pin1wire"])         { this->pin_1wire = (elem["pin1wire"].as<int>()) - 200;}
          if (elem["sel_oled"])         { if (strcmp(elem["sel_oled"], "none")==0) { this->enable_oled=false;} else {this->enable_oled=true;}}
          if (elem["sel_1wire"])        { if (strcmp(elem["sel_1wire"], "none")==0) { this->enable_1wire=false;} else {this->enable_1wire=true;}}
          if (elem["sel_3wege"])        { if (strcmp(elem["sel_3wege"], "none")==0) { this->enable_3wege=false;} else {this->enable_3wege=true;}}
          if (elem["i2coled"])          { this->i2caddress_oled = strtoul(elem["i2coled"], NULL, 16);} // hex convert to dec    
          if (elem["oled_type"])        { this->oled_type = elem["oled_type"].as<int>();} 
          if (elem["ventil3wege_port"]) { this->ventil3wege_port = elem["ventil3wege_port"].as<int>();}
          if (elem["serial_rx"])        { this->serial_rx = (elem["serial_rx"].as<int>()) - 200;}
          if (elem["serial_tx"])        { this->serial_tx = (elem["serial_tx"].as<int>()) - 200;}
        }
      } while (stream.findUntil(",","]"));
    } else {
      this->logN(1, "cannot open existing baseconfig.json config File, load default BaseConfig"); // -> constructor
    }
  } else {
    this->logN(1, "baseconfig.json config File not exists, load default BaseConfig");
  }

  // Data Cleaning
  if(this->mqtt_basepath.endsWith("/")) {
    this->mqtt_basepath = this->mqtt_basepath.substring(0, this->mqtt_basepath.length()-1); 
  }
}

const String BaseConfig::GetReleaseName() {
  return String(Release) + "(@" + String(GIT_BRANCH) + ")"; 
}

/* https://cpp4arduino.com/2018/11/06/what-is-heap-fragmentation.html*/
size_t BaseConfig::getFragmentation() {
  return 100 - ESP_GetMaxFreeAvailableBlock() * 100 / ESP.getFreeHeap();
}

void BaseConfig::GetInitData(JsonDocument& json) {
  std::ostringstream i2caddress_oled_hex;
  i2caddress_oled_hex << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)this->i2caddress_oled;

  json["data"].to<JsonObject>();
  json["data"]["mqttroot"]    = this->mqtt_root;
  json["data"]["mqttserver"]  = this->mqtt_server;
  json["data"]["mqttport"]    = this->mqtt_port;
  json["data"]["mqttuser"]    = this->mqtt_username;
  json["data"]["mqttpass"]    = this->mqtt_password;
  json["data"]["mqttbasepath"]= this->mqtt_basepath;
  json["data"]["debuglevel"]  = this->debuglevel;
  json["data"]["sel_URCID1"]  = ((this->mqtt_UseRandomClientID)?0:1);
  json["data"]["sel_URCID2"]  = ((this->mqtt_UseRandomClientID)?1:0);
  json["data"]["keepalive"] = this->keepalive;
  
  #ifdef ESP32
    json["data"]["sel_wifi"] = ((this->useETH)?0:1);
    json["data"]["sel_eth"]  = ((this->useETH)?1:0);
  #else
    json["data"]["tr_LAN"]["className"] = "hide";
    json["data"]["SelectLAN"]["className"] = "hide";
  #endif

  #ifdef USE_I2C
    json["data"]["GpioPin_0"] = this->pin_sda + 200;
    json["data"]["GpioPin_1"] = this->pin_scl + 200;
  #else 
    json["data"]["tr_sda"]["className"] = "hide";
    json["data"]["tr_scl"]["className"] = "hide";
  #endif

  #ifdef USE_ONEWIRE
    json["data"]["sel_ow1"] = ((this->enable_1wire)?0:1);
    json["data"]["sel_ow2"] = ((this->enable_1wire)?1:0);
    json["data"]["GpioPin_3"] = this->pin_1wire + 200;
  #else
    json["data"]["tr_owSelect"]["className"] = "hide";
    json["data"]["onewire_0"]["className"] = "hide";
  #endif

  #ifdef USE_WEBSERIAL
    json["data"]["tr_serial_rx"]["className"] = "hide";
    json["data"]["tr_serial_tx"]["className"] = "hide";
  #else
    json["data"]["GpioPin_serial_rx"] = this->serial_rx + 200;
    json["data"]["GpioPin_serial_tx"] = this->serial_tx + 200;
  #endif

  #ifdef USE_OLED
    json["data"]["sel_oled1"] = ((this->enable_oled)?0:1);
    json["data"]["sel_oled2"] = ((this->enable_oled)?1:0);
    json["data"]["i2caddress_oled"] = i2caddress_oled_hex.str();

    json["data"]["oled_rows"].to<JsonArray>();
    json["data"]["oled_rows"][0]["oled_row"].to<JsonObject>();
    json["data"]["oled_rows"][0]["oled_row"]["value"] = 0;
    json["data"]["oled_rows"][0]["oled_row"]["selected"] = (this->oled_type==0?"selected":"");
    json["data"]["oled_rows"][0]["oled_row"]["text"] = "OLED SSD1306";

    json["data"]["oled_rows"][1]["oled_row"].to<JsonObject>();
    json["data"]["oled_rows"][1]["oled_row"]["value"] = 1;
    json["data"]["oled_rows"][1]["oled_row"]["selected"] = (this->oled_type==1?"selected":"");
    json["data"]["oled_rows"][1]["oled_row"]["text"] = "OLED SH1106";
  #else
    json["data"]["tr_oledSelect"]["className"] = "hide";
    json["data"]["oled_0"]["className"] = "hide";
    json["data"]["oled_1"]["className"] = "hide";
  #endif

  json["data"]["sel_3wege_0"] = ((this->enable_3wege)?0:1);
  json["data"]["sel_3wege_1"] = ((this->enable_3wege)?1:0);
  json["data"]["ConfiguredPort_0"] = this->ventil3wege_port;
  
  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

void BaseConfig::logN(const int loglevel, const char* format, ...) {
  if (this->GetDebugLevel() < loglevel) return;
  
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  #ifdef USE_WEBSERIAL
    WebSerial.printf("[Log %d] ", loglevel);
    //if (this->GetDebugLevel() >= 4) { WebSerial.printf("FreeHeap: %d Bytes\n ", ESP.getFreeHeap()); }
    WebSerial.println(buffer);
  #else
    Serial.printf("[Log %d] ", loglevel);
    //if (this->GetDebugLevel() >= 4) { Serial.printf("FreeHeap: %d Bytes\n ", ESP.getFreeHeap()); }
    Serial.println(buffer);
  #endif
  va_end(args);
}

void BaseConfig::log(const int loglevel, const char* format, ...) {
  if (this->GetDebugLevel() < loglevel) return;
  
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  #ifdef USE_WEBSERIAL
    WebSerial.print(buffer);
  #else
    Serial.print(buffer);
  #endif
  va_end(args);
}

void BaseConfig::log(const int loglevel, const JsonDocument& json) {
  if (this->GetDebugLevel() < loglevel) return;
  
  #ifdef USE_WEBSERIAL
    serializeJsonPretty(json, WebSerial);
    WebSerial.println();
  #else
    serializeJsonPretty(json, Serial);
    Serial.println();
  #endif
}