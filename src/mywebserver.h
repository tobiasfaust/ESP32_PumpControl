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

#ifdef USE_FLOWCONTROL
  #include "flowcontrol.h"
  extern flowControl* FlowCtrl;
#endif
#ifdef USE_DOIF
  #include "doif.h"
#endif
#ifdef USE_FLOWERCARE
  #include "flowercare.h"
#endif

extern sensor* LevelSensor;
extern valveStructure* VStruct;
extern valveRelation* ValveRel;
extern flowControl* FlowCtrl;


#ifdef USE_I2C
  extern i2cdetect* I2Cdetect;
#endif

class MyWebServer {

  typedef struct {
    uint32_t ws_id;
    enum requestData_t {LOG_DATA, FLOWERCARE_DATA, ADS1115_DATA, FLOWCONTROL_DATA} requestData;
  } wsclient_t;

  public:
    MyWebServer(fs::LittleFSFS& sysFS, fs::LittleFSFS& configFS, AsyncWebServer *server, DNSServer* dns);
    void  loop();
    void  processCommand(JsonDocument& ret, String& jsonStr, uint32_t client_id = 0); // client_id is optional, default is for non-websocket commands

    #ifdef USE_DOIF
      void  DoIfOnMqttMessage(String& topic, String& msg);
    #endif
    
  private:

    AsyncWebServer* server;
    DNSServer* dns;
    AsyncWebSocket* ws;
    fs::LittleFSFS& sysFS;
    fs::LittleFSFS& configFS;

    bool      DoReboot;
    unsigned long RequestRebootTime;
    unsigned long ota_progress_millis = 0;
    std::vector<wsclient_t>* _wsclientRequests = NULL;

    handleFiles* fsfiles;
    
    #ifdef USE_DOIF
      doIf* doif;
      void      flowControlGetValuesCallback(JsonDocument& json, uint32_t wsclient_id);
    #endif

    #ifdef USE_FLOWERCARE
      flowercareWeb* flowerCare = nullptr;
      void      flowerCareGetValuesCallback(JsonDocument& json, uint32_t wsclient_id);
      void      flowerCareOnScanEndCallback();
    #endif

    void      LevelSensorGetValuesCallback(JsonDocument& json, uint32_t wsclient_id);
    void      logGetValuesCallback(const char* logline, JsonDocument& json, uint32_t wsclient_id);

    void      handleNotFound(AsyncWebServerRequest *request);
    void      handleRoot(AsyncWebServerRequest *request);
    bool      handleReset();
        
    void      GetInitDataStatus(JsonDocument& json);
    void      GetInitDataNavi(JsonDocument& json);
    
    void      onOTAStart();
    void      onOTAProgress(size_t current, size_t final);
    void      onOTAEnd(bool success);
    void      onImprovWiFiConnectedCb(const char *ssid, const char *password);
    void      onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

};

#endif
