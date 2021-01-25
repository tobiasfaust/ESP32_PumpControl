#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

//#if defined(ESP8266) || defined(ESP32)
//  #define min(x,y) _min(x,y)
//#endif

#ifdef ESP8266
  extern "C" {
      #include "user_interface.h"
  }
  #include <FS.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPUpdateServer.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266WiFi.h>
#elif ESP32
  #include "SPIFFS.h"
  #include <WiFi.h>
  #include <esp_wifi.h>  
  #include <Update.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
#endif

#ifdef ESP8266
  using WM_WebServer = ESP8266WebServer;
#elif ESP32
  using WM_WebServer = WebServer;
#endif
