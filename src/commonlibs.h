#ifndef COMMONLIBS_H
#define COMMONLIBS_H

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#ifdef ESP8266
  extern "C" {
      #include "user_interface.h"
  }

  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <FS.h>
#endif 

#ifdef ESP32
  #include <WiFi.h> 
  #include <AsyncTCP.h>
#endif

#include <string.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#ifdef USE_WEBSERIAL
  #include <WebSerial.h>
#endif

#if defined(USE_OLED) || defined(USE_PCF8574) || defined(USE_TB6612)
  #define USE_I2C
#endif

#ifdef ESP8266
  #define ESP_GetMaxFreeAvailableBlock() ESP.getMaxFreeBlockSize()
#else
  #define ESP_GetMaxFreeAvailableBlock() ESP.getMaxAllocHeap()
#endif

// Initialize littlefs data partitions  
extern fs::LittleFSFS sysFS;
extern fs::LittleFSFS configFS;

#endif