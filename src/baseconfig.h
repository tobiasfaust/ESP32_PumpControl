#ifndef BASECONFIG_H
#define BASECONFIG_H

#include "commonlibs.h"
#include "ArduinoJson.h"
#include <StreamUtils.h>
#include <iomanip>  // needed by setw / setfill
#include <sstream>
#include <vectorlist.h>
#include <_Release.h>

class BaseConfig {

  public:

    // Speaking identifiers
    enum class GpioIdentifier : uint8_t {
      BASECONFIG = 0,
      FLOWCONTROL,
      SENSOR,
      VALVES,
      ETH,
      OTHER
    };

    BaseConfig(fs::LittleFSFS& configFS);
    void      LoadJsonConfig();

    /**
    * @brief Wrapper function for logging like Serial.printf
    * @param format the format string
    * @param ... the arguments
    */
    void log(const int loglevel, const char* format, ...);
    void logN(const int loglevel, const char* format, ...);
    void log(const int loglevel, const JsonDocument& json);
    bool addWifiBssid(JsonDocument& json, bool ScanForWifi = false);

    // callbacks
    /************************
     * @brief Callback for getting the values
     * @param function(const char&) the callback function
     ************************/
    void onLogValues(std::function<void(const char*)> callback);

    const uint8_t&  GetPinSDA()      const {return pin_sda;}
    const uint8_t&  GetPinSCL()      const {return pin_scl;}
    const uint8_t&  GetPin1Wire()      const {return pin_1wire;}
    const uint8_t&  GetI2cOLED()     const {return i2caddress_oled;}
    const bool&     EnabledOled()    const {return enable_oled;}
    const uint8_t&  GetOledType()   const {return oled_type;}
    const bool&     Enabled1Wire()    const {return enable_1wire;}
    const String&   GetMqttServer()  const {return mqtt_server;}
    const uint8_t*  GetWifiBSSID()   const {return wifibssid;}
    const uint16_t& GetMqttPort()   const {return mqtt_port;}
    const String&   GetMqttUsername()const {return mqtt_username;}
    const String&   GetMqttPassword()const {return mqtt_password;}
    const String&   GetMqttBasePath()  const {return mqtt_basepath;}
    const String&   GetMqttRoot()    const {return mqtt_root;}
    const bool&     UseRandomMQTTClientID() const { return mqtt_UseRandomClientID; }
    const uint8_t&  Get3WegePort()   const {return ventil3wege_port;}
    const bool&     Enabled3Wege()   const {return enable_3wege;}
    const uint8_t&  GetMaxParallel() const {return max_parallel;}
    const uint16_t& GetKeepAlive()   const {return keepalive;}
    const uint8_t&  GetDebugLevel()   const {return debuglevel;}
    const String    GetReleaseName();
    const bool&     GetUseETH()        const { return useETH; }
    void            GetInitData(JsonDocument& json);
    const String&   GetLANBoard()      const {return LANBoard;}
    const uint8_t&  GetSerialRx()     const {return serial_rx;}
    const uint8_t&  GetSerialTx()     const {return serial_tx;}
    const uint8_t&  GetMaxThreads()   const {return max_threads;}

    size_t          getFragmentation();

  // Maintains a list of currently 'reserved' GPIOs (SDA, SCL, 1Wire, Serial, etc.)

  vectorlist<uint8_t, GpioIdentifier> disabledGPIO;

  private:
    fs::LittleFSFS configFS; 
    String    mqtt_server;
    String    mqtt_username;
    String    mqtt_password;
    uint16_t  mqtt_port;
    String    mqtt_root;
    String    mqtt_basepath;
    bool      mqtt_UseRandomClientID;
    uint16_t  keepalive;
    uint8_t   debuglevel;
    uint8_t   pin_sda;
    uint8_t   pin_scl;
    uint8_t   pin_1wire;
    bool      enable_oled;
    uint8_t   oled_type;
    bool      enable_1wire;
    uint8_t   i2caddress_oled;
    bool      enable_3wege; // wechsel Regen- /Trinkwasser
    uint8_t   ventil3wege_port; // Portnummer des Ventils
    uint8_t   max_parallel;
    bool      useETH;  // otherwise use WIFI
    String    LANBoard;
    uint8_t   serial_rx;
    uint8_t   serial_tx;
    uint8_t   max_threads;
    uint8_t   wifibssid[6] = {0};

    std::function<void(const char*)> onLogValuesCallback; // Callback function pointer

    void      setWifiBSSID(String bssid_str);
    bool      isWifiBssidSet();

};

extern BaseConfig* Config;

#endif
