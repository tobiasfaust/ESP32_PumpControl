#ifndef DOIF_H
#define DOIF_H

#include "commonlibs.h"
#include <vector>
#include <ArduinoJson.h>
#include "mymqtt.h"
#include "valveStructure.h"
#include "baseconfig.h"

class doIf {

  typedef struct {
    bool enabled;
    String TriggerTopic = "";
    String RegExpPattern = "";
    uint8_t ActorPort; 
    unsigned int threshold;
    unsigned int duration;
  } doIf_t;

  public:
    doIf(fs::LittleFSFS& configFS);
    
    void      LoadJsonConfig();
    void      GetInitData(JsonDocument& json);
    bool      setActive(String topic, uint8_t port, bool active);
    void      onMqttMessage(valveStructure* VStruct, String& topic, String& msg);

  private:
    fs::LittleFSFS configFS;
    std::vector<doIf_t>* _relationen  = NULL;
};

#endif
