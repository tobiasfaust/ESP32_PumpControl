#ifndef BASECONFIG_H
#define BASECONFIG_H

//#if defined(ESP8266) || defined(ESP32)
//  #define min(x,y) _min(x,y)
//#endif

#include "CommonLibs.h"
#include "ArduinoJson.h"
#include "oled.h"
#include "updater.h"

extern OLED* oled;

class BaseConfig {

  public:
    BaseConfig();
    void      StoreJsonConfig(String* json); 
    void      LoadJsonConfig();
    void      GetWebContent(WM_WebServer* server);
    void      loop();
    const uint8_t& GetPinSDA()      const {return pin_sda;}
    const uint8_t& GetPinSCL()      const {return pin_scl;}
    const uint8_t& GetPin1Wire()      const {return pin_1wire;}
    const uint8_t& GetI2cOLED()     const {return i2caddress_oled;}
    const bool&    EnabledOled()    const {return enable_oled;}
    const bool&    Enabled1Wire()    const {return enable_1wire;}
    const String&  GetMqttServer()  const {return mqtt_server;}
    const uint16_t& GetMqttPort()   const {return mqtt_port;}
    const String&  GetMqttUsername()const {return mqtt_username;}
    const String&  GetMqttPassword()const {return mqtt_password;}
    const String&  GetMqttRoot()    const {return mqtt_root;}
    const bool&    UseRandomMQTTClientID() const { return mqtt_UseRandomClientID; }
    const uint8_t& Get3WegePort()   const {return ventil3wege_port;}
    const bool&    Enabled3Wege()   const {return enable_3wege;}
    const uint8_t& GetMaxParallel() const {return max_parallel;}
    const uint16_t& GetKeepAlive()   const {return keepalive;}
    const uint8_t& GetDebugLevel()   const {return debuglevel;}
    String    GetReleaseName();
    void      InstallRelease(uint32_t ReleaseNumber);
    void      RefreshReleases();
    
  private:
    String    mqtt_server;
    String    mqtt_username;
    String    mqtt_password;
    uint16_t  mqtt_port;
    String    mqtt_root;
    bool      mqtt_UseRandomClientID;
    uint8_t   pin_sda;
    uint8_t   pin_scl;
    uint8_t   pin_1wire;
    bool      enable_oled;
    bool      enable_1wire;
    bool      enable_autoupdate;
    String    autoupdate_url;
    stage_t   autoupdate_stage;
    uint8_t   i2caddress_oled;
    bool      enable_3wege; // wechsel Regen- /Trinkwasser
    uint8_t   ventil3wege_port; // Portnummer des Ventils
    uint8_t   max_parallel;
    uint16_t  keepalive;
    uint8_t   debuglevel;

    updater*  ESPUpdate;
};

#endif
