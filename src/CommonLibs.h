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
#elif ESP32
  #include <WiFi.h> 
  #include <AsyncTCP.h>
#endif

#include <string.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#ifdef USE_WEBSERIAL
  #include <WebSerial.h>
  #define dbg WebSerial
#else
  #define dbg Serial1  
#endif

#if defined(USE_OLED) || defined(USE_PCF8574) || defined(USE_TB6612)
  #define USE_I2C
#endif

#define AWS_URL "http://tfa-releases.s3-website.eu-central-1.amazonaws.com"

#ifdef ESP8266
  #define MY_ARCH "ESP8266"
#elif ARDUINO_ESP32_DEV
  #define MY_ARCH "ESP32"
#elif ARDUINO_ESP32S2_DEV
  #define MY_ARCH "ESP32-S2"
#elif ARDUINO_ESP32S3_DEV
  #define MY_ARCH "ESP32-S3"
#elif ARDUINO_ESP32C3_DEV
  #define MY_ARCH "ESP32-C3"
#endif

#ifdef ESP8266
  #define ESP_getChipId() ESP.getChipId() 
  #define ESP_GetMaxFreeAvailableBlock() ESP.getMaxFreeBlockSize()
#elif ESP32
  #define ESP_getChipId() (uint32_t)ESP.getEfuseMac()   // Unterschied zu ESP.getFlashChipId() ???
  #define ESP_GetMaxFreeAvailableBlock() ESP.getMaxAllocHeap()
#endif


#endif