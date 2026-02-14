
#ifndef VALVESTRUCTURE_H
#define VALVESTRUCTURE_H

#include "commonlibs.h"
#include <ArduinoJson.h>
#include "baseconfig.h"
#include "valveRelation.h"
#include "valve.h"
#include "mymqtt.h"

extern BaseConfig* Config;
extern valveRelation* ValveRel;

#ifdef USE_I2C
  #include <i2cdetect.h>
  extern i2cdetect* I2Cdetect;
#endif

class valveStructure {

  struct waitingQueue_t {
    uint8_t       Port;
    unsigned int  duration;
    // Equality operator needed for push_back_unique / std::find
    bool operator==(const waitingQueue_t& other) const {
      return Port == other.Port;
    }
  };

  public:
    valveStructure(fs::LittleFSFS& configFS, uint8_t sda, uint8_t scl);
    void      loop();
    void      OnForTimer(String SubTopic, unsigned int duration);
    void      OnForTimer(uint8_t Port, unsigned int duration);
    void      OnForTimer(valve* valve, unsigned int duration);
    void      SetOn(String SubTopic);
    void      SetOn(uint8_t Port);
    void      SetOn(valve* valve);
    void      SetOff(String SubTopic);
    void      SetOff(uint8_t Port);
    void      SetOff(valve* valve);
    bool      GetState(uint8_t Port);
    bool      GetEnabled(uint8_t Port);
    void      SetEnable(uint8_t Port, bool state);
    uint8_t   CountActiveThreads();
    
    void      GetInitData(JsonDocument& json);
    void      GetInitData1Wire(JsonDocument& json);

    void      LoadJsonConfig();
    void      getWebJsParameter(JsonDocument& json);
    void      ReceiveMQTT(String topic, unsigned int value);
    uint8_t   Get1WireCountDevices();
    uint8_t   Refresh1WireDevices();
    
  private:
    fs::LittleFSFS configFS;
    valve*    GetValveItem(uint8_t Port);
    valve*    GetValveItem(String SubTopic);
    void      handleDeps(String topic, unsigned int value); //prueft die Relationen

    valveHardware* ValveHW = NULL;
    std::shared_ptr<std::vector<valve>> Valves;
    std::vector<waitingQueue_t>* waitingQueue = nullptr;

    uint8_t pin_sda = SDA;
    uint8_t pin_scl = SCL;
    uint8_t parallelThreads = 0;
};

#endif