#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H

#include "commonlibs.h" 
#include <ArduinoJson.h>
#include "uptime.h" // https://github.com/YiannisBourkelis/Uptime-Library/
#include "uptime_formatter.h"
#include "_Release.h"
#include "handleFiles.h"

#include "baseconfig.h"
#include "sensor.h"
#include "valveStructure.h"
#include "valveRelation.h"

#include <ElegantOTA.h>

#ifdef USE_FLOWERCARE
  #include "flowercare.h"
#endif

extern sensor* LevelSensor;
extern valveStructure* VStruct;
extern valveRelation* ValveRel;

#ifdef USE_I2C
  extern i2cdetect* I2Cdetect;
#endif

class MyWebServer {

  public:
    MyWebServer(AsyncWebServer *server, DNSServer* dns);

    void      loop();

  private:

    AsyncWebServer* server;
    DNSServer* dns;
    AsyncWebSocket* ws;

    bool      DoReboot;
    unsigned long RequestRebootTime;
    unsigned long ota_progress_millis = 0;

    handleFiles* fsfiles;

    #ifdef USE_FLOWERCARE
      FlowerCare* flowerCare = nullptr;
    #endif

    
    void      handleNotFound(AsyncWebServerRequest *request);
    void      handleRoot(AsyncWebServerRequest *request);
    bool      handleReset();
        
    void      GetInitDataStatus(JsonDocument& json);
    void      GetInitDataNavi(JsonDocument& json);

    #ifdef USE_FLOWERCARE
    void      GetInitDataFlowerCare(JsonDocument& json);
    #endif
    
    void      onOTAStart();
    void      onOTAProgress(size_t current, size_t final);
    void      onOTAEnd(bool success);
    void      onImprovWiFiConnectedCb(const char *ssid, const char *password);
    void      onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
    void      flowerCareGetValuesCallback(JsonDocument& json);
};

#endif
