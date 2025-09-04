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
#include "flowcontrol.h"

#include <ElegantOTA.h>

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

  typedef struct {
    bool enabled;
    String TriggerTopic = "";
    uint8_t ActorPort; 
    unsigned int threshold;
    unsigned int duration;
  } FlowercareRelation_t;

  public:
    MyWebServer(fs::LittleFSFS& sysFS, fs::LittleFSFS& configFS, AsyncWebServer *server, DNSServer* dns);
    void  loop();

    void  flowerCareOnMqttMessage(String& topic, String& JsonMsg);

  private:

    AsyncWebServer* server;
    DNSServer* dns;
    AsyncWebSocket* ws;
    fs::LittleFSFS& sysFS;
    fs::LittleFSFS& configFS;

    bool      DoReboot;
    unsigned long RequestRebootTime;
    unsigned long ota_progress_millis = 0;
    std::vector<FlowercareRelation_t>* _relationen  = NULL;
    std::vector<wsclient_t>* _wsclientRequests = NULL;

    handleFiles* fsfiles;

    #ifdef USE_FLOWERCARE
      FlowerCare* flowerCare = nullptr;
      void      flowerCareGetValuesCallback(JsonDocument& json, uint32_t wsclient_id);
      void      flowerCareOnScanEndCallback();
    #endif

    void      flowControlGetValuesCallback(JsonDocument& json, uint32_t wsclient_id);
    void      LevelSensorGetValuesCallback(JsonDocument& json, uint32_t wsclient_id);

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

    void      GetInitDataFlowerCare(JsonDocument& json);
    void      LoadFlowerCareConfig();      
};

#endif
