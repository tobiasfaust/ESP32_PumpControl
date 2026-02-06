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
#include <helper.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#if defined(USE_OLED) || defined(USE_PCF8574) || defined(USE_TB6612)
  #define USE_I2C
#endif

#ifndef DEFAULT_I2C_SDA_PIN
  #define DEFAULT_I2C_SDA_PIN 32
#endif
#ifndef DEFAULT_I2C_SCL_PIN
  #define DEFAULT_I2C_SCL_PIN 33
#endif
#ifndef DEFAULT_VALVE1_PIN
  #define DEFAULT_VALVE1_PIN 13
#endif
#ifndef DEFAULT_VALVE2_PIN
  #define DEFAULT_VALVE2_PIN 17
#endif

#ifdef ESP8266
  #define ESP_GetMaxFreeAvailableBlock() ESP.getMaxFreeBlockSize()
#else
  #define ESP_GetMaxFreeAvailableBlock() ESP.getMaxAllocHeap()
#endif

#endif