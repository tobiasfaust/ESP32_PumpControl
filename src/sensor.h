#ifndef SENSOR_H
#define SENSOR_H

#include "CommonLibs.h"
#include <ArduinoJson.h>
#include <vector>
#include "MyMqtt.h"
#include "valveStructure.h"
#include "baseconfig.h"

#ifdef USE_ADS1115
  #include <ADS1115_WE.h> 

  typedef struct {
    ADS1115_WE device;
    uint8_t i2cAddress;
    String topic_chan1;
    String topic_chan2;
    String topic_chan3;
    String topic_chan4;
  } adsdev_t;

#endif

#ifdef USE_OLED
  #include "oled.h"
#endif

extern valveStructure* VStruct;
extern BaseConfig* Config;

enum sensorType_t {NONE, EXTERN, HCSR04, ONBOARD_ANALOG, ADS1115};

class sensor {

  public:
    sensor();
    void      init_hcsr04(uint8_t pinTrigger, uint8_t pinEcho);
    void      init_extern(String externalSensor);
    void      init_analog(uint8_t pinAnalog) ;
    
    #ifdef USE_ADS1115
      void      init_ads1115(uint8_t i2c, uint8_t port, String topic="");
    #endif

    void      setSensorType(sensorType_t t);
    void      loop();
    void      SetLvl(uint8_t lvl);
    void      LoadJsonConfig();
    void      GetInitData(AsyncResponseStream* response);    
    
    const uint16_t& GetRaw() const {return raw;}
    const uint8_t&  GetLvl() const {return level; }
    const sensorType_t& GetType() const {return Type; }
    const uint8_t& GetThresholdMin()const {return threshold_min;}
    const uint8_t& GetThresholdMax()const {return threshold_max;}
    const String&  GetExternalSensor() const {return externalSensor;}
 
    #ifdef USE_OLED
      void      SetOled(OLED* oled);
    #endif

  private:
    void      loop_analog();
    void      loop_hcsr04();
    
    #ifdef USE_ADS1115
      std::vector<adsdev_t>* ads1115_devices  = NULL;
      adsdev_t* getAdsDevice(uint8_t i2c);
      ADS1115_MUX getAdsChannel(uint8_t port);

      uint16_t  readADS1115Channel(adsdev_t* device, ADS1115_MUX channel);
      void      loop_ads1115_sensor();
      void      loop_ads1115_moisture();
    #endif

    sensorType_t   Type;
    
    uint16_t  measureDistMin;
    uint16_t  measureDistMax;
    uint16_t  measurecycle;
    uint8_t   level;
    uint16_t  raw;
    uint8_t   pinTrigger;
    uint8_t   pinEcho;
    uint8_t   pinAnalog;
    uint8_t   ads1115_i2c;
    uint8_t   ads1115_port; //0..3
    uint16_t  MAX_DIST;
    uint8_t   threshold_min;
    uint8_t   threshold_max;
    String    externalSensor;
    bool      moistureEnabled;
    
    unsigned long previousMillis_sensor = 0;
    unsigned long previousMillis_moisture = 0;

    #ifdef USE_OLED
      OLED*    oled;
    #endif
    
};

#endif
