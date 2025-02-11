#ifndef MYMQTT_H
#define MYMQTT_H

#include "CommonLibs.h" 
#include <PubSubClient.h>
#include <vector>
#include <mqtt.h>
#include "baseconfig.h"

#ifdef USE_OLED
  #include "oled.h"
#endif

class MyMQTT: public MQTT {
  
  public:
    enum MqttSubscriptionType_t {RELATION, SENSOR};
    
    typedef struct {
      String subscription = "";
      MqttSubscriptionType_t identifier;
      bool active;
    } subscription_t;

    MyMQTT(const char* MqttServer, uint16_t MqttPort, String MqttBasepath, String MqttRoot);
  
    void    loop();
    void    Subscribe(String topic, MqttSubscriptionType_t identifier);
    void    ClearSubscriptions(MqttSubscriptionType_t identifier);

    #ifdef USE_OLED
      void    SetOled(OLED* oled);
    #endif

  private:
    std::vector<subscription_t>* subscriptions = NULL;

    #ifdef USE_OLED
      OLED*    oled;
    #endif

    void      reSubscribe();
};

extern MyMQTT* mqtt;

#endif
