#include <vector>
#include "commonlibs.h"
#include "baseconfig.h"
#include "valveStructure.h"
#include "mymqtt.h"
#include "mywebserver.h"
#include "sensor.h"

#ifdef USE_FLOWERCARE
  #include "flowercare.h"
#endif

#ifdef USE_OLED
  #include "oled.h"
  OLED* oled = nullptr;
#endif

#ifdef USE_I2C
  i2cdetect* I2Cdetect = nullptr;
#endif

AsyncWebServer server(80);
DNSServer dns;

BaseConfig* Config = nullptr;
valveRelation* ValveRel = nullptr;
valveStructure* VStruct = nullptr;
MyMQTT* mqtt = nullptr;
sensor* LevelSensor = nullptr;
MyWebServer* mywebserver = nullptr;

#ifdef USE_FLOWERCARE
  FlowerCare* flowerCare = nullptr;
#endif

/* debugmodes --> in der WebUI -> Basisconfig einstellbar
    0 -> nothing
    1 -> major and criticals
    2 -> majors
    3 -> standard
    4 -> more details, plus: available RAM, RSSI via MQTT, WiFi Credentials via Serial
    5 -> max details
*/

void myMQTTCallBack(char* topic, byte* payload, unsigned int length) {
  String msg;
  
  for (u_int16_t i = 0; i < length; i++) {
    msg.concat((char)payload[i]);
  }
  
  Config->logN(4, "Message arrived [%s]\nMessage: %s", topic, msg.c_str()); 

  if (LevelSensor->GetExternalSensor() && (strcmp(LevelSensor->GetExternalSensor().c_str(), topic)==0)) {
    LevelSensor->SetLvl(atoi(msg.c_str()));
  }
  else if (strstr(topic, "/raw") ||  strstr(topic, "/level") ||  strstr(topic, "/mem") ||  strstr(topic, "/rssi")) {
    /*SensorMeldungen - ignore!*/
  }
  else {
    VStruct->ReceiveMQTT((String)topic, atoi(msg.c_str()));
  }
}

void setup() {
  #ifdef ESP8266
    LittleFS.begin();
  #elif ESP32
    LittleFS.begin(true); // true: format LittleFS/NVS if mount fails
  #endif
  
  // Flash Write Issue
  // https://github.com/esp8266/Arduino/issues/4061#issuecomment-428007580
  //LittleFS.format();

  Config = new BaseConfig();
  
  #ifndef USE_WEBSERIAL
    Serial.begin(115200,
                 SERIAL_8N1,
                 Config->GetSerialRx(),
                 Config->GetSerialTx());  // RX, TX, zb.: 33, 32
    Serial.println("");
    Serial.println("ready");
  #endif

  #ifdef USE_WEBSERIAL
    WebSerial.onMessage([](const String& msg) { Serial.println(msg); });
    WebSerial.begin(&server);
    WebSerial.setBuffer(100);
  #endif

  Config->logN(1, "Start of ESP PumpControl");

  #ifdef USE_I2C
    Config->logN(1, "Starting WIRE at (SDA, SCL)): %d, %d ", Config->GetPinSDA(), Config->GetPinSCL());
    Wire.begin(Config->GetPinSDA(), Config->GetPinSCL());
  #endif

  #ifdef USE_OLED
    oled = new OLED();    
    if (Config->EnabledOled() ) oled->init(Config->GetPinSDA(), Config->GetPinSCL(), Config->GetI2cOLED());
    oled->Enable(Config->EnabledOled());
  #endif

  Config->logN(1, "Starting Wifi and MQTT");
  mqtt = new MyMQTT(Config->GetMqttServer().c_str(),
                    Config->GetMqttPort(),
                    Config->GetMqttBasePath().c_str(),
                    Config->GetMqttRoot().c_str());
  mqtt->setCallback(myMQTTCallBack);
  
  #ifdef USE_OLED
    mqtt->SetOled(oled);
  #endif

  #ifdef USE_I2C
    Config->logN(1, "Starting I2CDetect");
    I2Cdetect = new i2cdetect(Config->GetPinSDA(), Config->GetPinSCL());
  #endif
  
  Config->logN(1, "Starting Sensor");
  LevelSensor = new sensor();
  #ifdef USE_OLED
    LevelSensor->SetOled(oled);
  #endif

  Config->logN(1, "Starting Valve Relations");
  ValveRel = new valveRelation();

  Config->logN(1, "Starting Valve Structure");
  VStruct = new valveStructure(Config->GetPinSDA(), Config->GetPinSCL());

  #ifdef USE_FLOWERCARE
    Config->logN(1, "Starting FlowerCare");
    flowerCare = new FlowerCare();
  #endif

  Config->logN(1, "attempting to start WebServer");
  mywebserver = new MyWebServer(&server, &dns);

  //VStruct->OnForTimer("Valve1", 10); // Test

  Config->logN(1, "Setup finished");
}

void loop() {
  VStruct->loop();
  mqtt->loop();
  LevelSensor->loop();
  mywebserver->loop();

  #ifdef USE_FLOWERCARE
    if (flowerCare) {
      flowerCare->loop();
    }
  #endif
  
  #ifdef USE_OLED
    oled->loop();  
  #endif

  #ifdef USE_WEBSERIAL
    WebSerial.loop();
  #endif
}
