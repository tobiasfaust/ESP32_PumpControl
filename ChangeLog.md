Release 3.0.1:
  - add: initialize bistable valves at start to prevent open valves
  - add: configurable serial output pins
  - improve wifi reconnect handling
  
Release 3.0.0:
  - +++++++ NO SUPPORT FOR ESP8266 anymore +++++++
  - change to Async Webserver
  - change ArduinoJson version 5.x to 6.x
  - change platform from Arduino-IDE to PlatformIO
  - reduce memory usage for handling json configs
  - derive custom mqtt handling from parent mqtt class
  - extract html-code into separate html-files, so everyone can customize his own instance
  - interact with Web-frontend by json
  - enable update filesystem at updatepage
  - saving configs by global upload function, no extra saveconfigfile functions anymore
  - data partition is now larger
  - change deprecated SPIFFS to LittleFS
  - move all webfiles (css,js) to FS
  - create new Webpage to maintain the FS-files, editing json registers on-the-fly is now possible
  - add ADS1115 moisture functionality
  - add page loader

Release 2.5.3:
  - Bug: Oled Typ selectionbox in GUI will now list correct
  - Bug: Reverse function works now as expected
  - ReScan i2cBus or 1WireBus will now change textcolor to red
  - Bug: ADS1115 Scan checks now if ADC is present to prevent ESP freezes 
  - Feature: 1Wire ReScan at statuspage
  - Bug: bistable valve works again due an definition error in 2.5.2
  - Bug: ESP8266: reduce autoupdater to last version due RAM limitations by changing json file definition
  - Bug: valve sometimes doesnt switch back to OFF status after on-for-timer 
  - Bug: in ValveConfig Ajax Change of enabling/disabling of a valve doesn recognized
  - Bug: 1Wire switches only one port
  
Release 2.5.2:
  - changing 1wire pin without reboot now possible
  - show 1wire devices and controller at status page
  - Feature: OLED Type SSD1306 and SH1106 available
  - add ADS1115 ADC for a better level measurement (page sensor)
  - debugmode handling optimized (-> use it only from BaseConfig)
  
Release 2.5.1:
  - Fix Firmware OTA Support for ESP32
  - Fix deleting stored WiFi Credentials
  - fixes some ESP32 bugs
  - refreshing i2c Seach in Basisconfig now working
  
Release 2.5.0:
  - Feature: Add OneWire DS2408 hardware support
  - Feature: Add a keepalive message via MQTT, configurable at BasisConfig: 
  - Feature: Add configurable DebugMode at BaseConfig
  - Feature: push out memory and rssi values via mqtt together with keepalive message if Debugmode >= 4
  - Feature: support for ESP32, still without OTA Update and Wifi Credential deletion
  
Release 2.4.5:
  - Feature: deletion of WiFi credentials now possible
  - Feature: ESP Hostname now the configured Devicename
  - Bug: WIFI Mode forces to STATION-Mode, some devices has been ran in unsecured STA+AP Mode
  - Bug: security issue: dont show debug output of WiFi Connection (password has been shown)
  - Feature: valve reverse mode: enable if your valve act on LOW instead of ON
  - Feature: AutoOff: possibility to setup a security AutoOff 
  - Bug: count of Threads now push out if an on-for-timer has been expired
  
Release 2.4.4:
  - Feature: Issue #9: MQTT Client ID now configurable
  - MQTT now reconnect after DeviceName has been changed
  - MQTT LastWillTopic as device status configured by topic "/state [Offline|Online]"
  - Publish Release and Version after MQTT Connect by topic "/version"
  - Bugfix: Nullpointer to Hardwaredevice if multiple hardware devices are defined

Release 2.4.3:
  - Bugfixing Automatische Releaseverteilung
  - Überarbeitung Github Workflow mit automatischer Releaseerstellung 

Release 2.4.2:
  - Bugfixing des TB6612 Handlings

Release 2.4.1:
  - Added TB6612 Support
  - Added automatic Release Update

Release 2.3:
  - solved some bugfixes
  - MQTT Commands setstate [on|off] now available

Release 2.2:
  - some bugs resolved

Release 2.1:
  Final Release! complete redesign, Its now easier to understand and add more functionality.
  Wiki is now up-to-date based on Release 2.1

  New functionality:
    - i2c Motordriver support for bistable valves
      - ESP8266 motordriverboard for bistable valves
      - Relations now added for complex garden
      - external and analog sensor support
      - changing valve status by Web-UI added

Release 2.0:
  1st Pre Release with completely new refactored code by completely class based.
  Tested with valves at PCF8575 and GPIO, LevelSensor HCSR04 and OLED 1306

Release 1.0:
  this is the finale on first release. Works with optional OLED, optional LevelSensor.
  Supports valves at OnBoard GPIO Pins and PCF8574 Extender i2c-Shield
