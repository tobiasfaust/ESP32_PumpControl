#ifndef FLOWCONTROL_H
#define FLOWCONTROL_H

#include "commonlibs.h"
#include <vector>
#include <ArduinoJson.h>
#include "mymqtt.h"

class flowControl {

  typedef struct {
    bool enabled;
    String name = "";
    uint8_t port; 
    unsigned int ImpPerLitre;
    volatile uint32_t count;
  } flowcontrol_t;
  
  public:
    flowControl(fs::LittleFSFS& configFS);

    void      loop();
    void      LoadJsonConfig();
    void      GetInitData(JsonDocument& json);
    
  private:
    fs::LittleFSFS configFS;
    static std::vector<flowcontrol_t> flowControlItems;
    static void IRAM_ATTR isrHandler(void* arg);
    static int findIndexByGpio(gpio_num_t gpio);
    static int findIndexByName(const String& name);

    void      AddFlowControl(bool enabled, String name, uint8_t port, unsigned int ImpPerLitre);
    void      DelFlowControl(uint8_t port);
    void      DelFlowControl();

    unsigned long lastCalculationTime;
    unsigned int  calculationPeriod = 5000;

};

#endif
