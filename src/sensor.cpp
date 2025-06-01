#include "sensor.h"

sensor::sensor(fs::LittleFSFS& configFS) :
  configFS(configFS), 
  Type(NONE), 
  measureDistMin(0), 
  measureDistMax(0), 
  measurecycle(10), 
  level(0), 
  raw(0),
  pinTrigger(5),
  pinEcho(6),
  ads1115_i2c(48),
  ads1115_port(0),
  threshold_min(26),
  threshold_max(30),
  moistureEnabled(false) {
  
  #ifdef ESP8266
    uint8_t pinAnalogDefault = 0;
  #endif

  #ifdef ESP32
    uint8_t pinAnalogDefault = 36; // ADC1_CH0 (GPIO 36) 
  #endif
  
  #ifdef USE_ADS1115
    ads1115_devices = new std::vector<adsdev_t>{};
  #endif

  this->pinAnalog = pinAnalogDefault;
  
  LoadJsonConfig(); 
}

void sensor::init_analog(uint8_t pinAnalog) {
  setSensorType(ONBOARD_ANALOG);
  this->pinAnalog = pinAnalog;
  this->MAX_DIST=500; // is maximum by default
}

void sensor::init_hcsr04(uint8_t pinTrigger, uint8_t pinEcho) {
  setSensorType(HCSR04);
  this->MAX_DIST = 23200; // Anything over 400 cm (400*58 = 23200 us pulse) is "out of range"
  this->pinTrigger = pinTrigger;
  this->pinEcho = pinEcho;
  pinMode(this->pinTrigger, OUTPUT);
  pinMode(this->pinEcho, INPUT);
}

void sensor::init_extern(String externalSensor) {
  this->setSensorType(EXTERN);
  this->measurecycle = 10;
  mqtt->Subscribe(externalSensor, MyMQTT::SENSOR);
}

#ifdef USE_OLED
void sensor::SetOled(OLED* oled) {
  this->oled = oled;
}
#endif

void sensor::setSensorType(sensorType_t t) {
  this->Type = t;
}

void sensor::SetLvl(uint8_t lvl) {
  Config->logN(4, "Sensor: Set Level from extern: %d", lvl);
  this->level = lvl;
  #ifdef USE_OLED
    if(this->oled) this->oled->SetLevel(this->level);
  #endif
}

void sensor::loop_analog() {
  this->raw = 0;
  this->level = 0;
  uint8_t pinanalog = this->pinAnalog;

  #ifdef ESP8266
    pinanalog = A0;;
  #endif

  Config->logN(4, "start measure, using analog Sensor pin: %d ", pinanalog);

  this->raw = analogRead(pinanalog);
  
  this->level = map(this->raw, measureDistMin, measureDistMax, 0, 100); // 0-100%
}

void sensor::loop_hcsr04() {
  this->raw = 0;
  this->level = 0;
  
  digitalWrite(this->pinTrigger, LOW);
  delayMicroseconds(2);

  digitalWrite(this->pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(this->pinTrigger, LOW);

  this->raw = pulseIn(this->pinEcho, HIGH, MAX_DIST); 
  this->raw = (this->raw / 2) / 29.1; //Distance in CM's, use /148 for inches.

  if (this->raw == 0){//Reached timeout
    Config->logN(2, "Out of range");
  } else {
    if (this->measureDistMax - this->measureDistMin > 0) {
      this->level = (((this->measureDistMax - this->raw)*100)/(this->measureDistMax - this->measureDistMin));
    }
  }
}

#ifdef USE_ADS1115
  void sensor::init_ads1115(uint8_t i2c, uint8_t port, String topic) {  
    adsdev_t* device = this->getAdsDevice(i2c);
    
    if (device == nullptr) { 
      Config->logN(4, "Init ADS1115 at i2cAdress 0x%02x ", i2c);
      
      adsdev_t ads;
      ads.device = ADS1115_WE(i2c);
      
      if(!ads.device.init()) {
        Config->logN(1, "Could not connect to ADS1115 at i2cAddress 0x%02x, ignore it!", i2c );
      } else {
        Config->logN(3, "Initialize ADS1115 at i2cAddress 0x%02x with channel %d", i2c, port);
        ads.device.setVoltageRange_mV(ADS1115_RANGE_4096);
        ads.i2cAddress = i2c;
        if (port == 0) {ads.topic_chan1 = topic;}
        if (port == 1) {ads.topic_chan2 = topic;}
        if (port == 2) {ads.topic_chan3 = topic;}
        if (port == 3) {ads.topic_chan4 = topic;}
        this->ads1115_devices->push_back(ads);
      }
    } else {
      Config->logN(3, "Add Channel %d to ADS1115 at i2cAddress 0x%02x with topic '%s' ", port, i2c, topic.c_str());
      if (port == 0) {device->topic_chan1 = topic;}
      if (port == 1) {device->topic_chan2 = topic;}
      if (port == 2) {device->topic_chan3 = topic;}
      if (port == 3) {device->topic_chan4 = topic;}
    }
  }

  
  /*returns the proper ADS1115 vector of given i2c-address */
  adsdev_t* sensor::getAdsDevice(uint8_t i2c) {
    for (uint8_t i=0; i<this->ads1115_devices->size(); i++) {
      if (this->ads1115_devices->at(i).i2cAddress == i2c) {
        return &this->ads1115_devices->at(i);
      }
    }
    return nullptr;
  }

  /*returns the proper ADS1115_MUX channel of given port number*/
  ADS1115_MUX sensor::getAdsChannel(uint8_t port) {
    switch (port) {
      case 0:
        return ADS1115_COMP_0_GND;
        break;
      case 1:
        return ADS1115_COMP_1_GND;
        break;
      case 2:
        return ADS1115_COMP_2_GND;
        break;
      case 3:
        return ADS1115_COMP_3_GND;
        break;
      default:
        Config->logN(2, "ADS115 portnummer %d not available ", port);
        break;
    }
    return ADS1115_COMP_0_GND;
  }

  void sensor::loop_ads1115_moisture() {
    uint16_t raw = 0;
    uint8_t  level = 0;
    
    for (uint8_t i=0; i<this->ads1115_devices->size(); i++) {
      for (uint8_t chan=0; chan<4; chan++) {
        if (this->ads1115_devices->at(i).i2cAddress == this->ads1115_i2c && chan == this->ads1115_port) {
          // exclude sensor channel from measurement the moisture
          continue;
        }  
        if (chan == 0 && this->ads1115_devices->at(i).topic_chan1.length()==0) continue;  
        if (chan == 1 && this->ads1115_devices->at(i).topic_chan2.length()==0) continue;
        if (chan == 2 && this->ads1115_devices->at(i).topic_chan3.length()==0) continue;
        if (chan == 3 && this->ads1115_devices->at(i).topic_chan4.length()==0) continue;

        raw = readADS1115Channel(&this->ads1115_devices->at(i), this->getAdsChannel(chan));
        /* map voltage to a moisture-level
        *  0-3.3V -> 0-100%
        *  moisture sensor gets 100% = dry, but we want a moisture: 100% = wet  
        */
        level = 100 - map(raw, 0, 3300, 0, 100); // 0-3.3V -> 0-100%
        Config->logN(4, "read moisture of ADS1115 (0x%02x) channel %d: raw: %d, calculated level: %d", this->ads1115_devices->at(i).i2cAddress, chan, raw, level);

        String topic = "";
        switch (chan) {
          case 0:
            topic = this->ads1115_devices->at(i).topic_chan1;
            break;
          case 1:
            topic = this->ads1115_devices->at(i).topic_chan2;
            break;
          case 2:
            topic = this->ads1115_devices->at(i).topic_chan3;
            break;
          case 3:
            topic = this->ads1115_devices->at(i).topic_chan4;
            break;
        } 

        if (raw > 0 ) { mqtt->Publish_Int(topic.c_str(), (int)level, false); }
      }
    }
  }

  void sensor::loop_ads1115_sensor() {
    this->raw = 0;
    this->level = 0;

    if (!this->getAdsDevice(this->ads1115_i2c)) {
      Config->logN(3, "Measure of analog Sensor ADS1115 port %d requested, but not ADS1115 found. Stop measure! ", this->ads1115_port);
    } else {

      Config->logN(4, "start measure, use analog Sensor ADS1115 port: %d ", this->ads1115_port);
    
      this->raw = readADS1115Channel(this->getAdsDevice(this->ads1115_i2c), this->getAdsChannel(this->ads1115_port));
      
      this->level = map(this->raw, measureDistMin, measureDistMax, 0, 100); // 0-100%
    }
  }

  uint16_t sensor::readADS1115Channel(adsdev_t* device, ADS1115_MUX channel) {
    int16_t raw = 0;
    
    device->device.setCompareChannels(channel);
    device->device.startSingleMeasurement();
    while(device->device.isBusy()){}
    raw = device->device.getResultWithRange(-4096,4096, 3300); 
    return (uint16_t) abs(raw);
  }
#endif

void sensor::loop() {
  /*start measuring soil moisture, every 60sec */
#ifdef USE_ADS1115
  if (millis() - this->previousMillis_moisture > 5*1000) {
    this->previousMillis_moisture = millis();
    loop_ads1115_moisture();
  }
#endif

  /*start measuring sensor*/
  if (millis() - this->previousMillis_sensor > this->measurecycle*1000) {
    this->previousMillis_sensor = millis();
   
    if (this->Type == ONBOARD_ANALOG) {loop_analog();}
   
    if (this->Type == HCSR04) {loop_hcsr04();}

    #ifdef USE_ADS1115
      if (this->Type == ADS1115) {loop_ads1115_sensor();}
    #endif

    if (this->Type != NONE && this->level !=0 && Config->Enabled3Wege()) {
      if (this->level < this->threshold_min) { VStruct->SetOn(Config->Get3WegePort()); }
      if (this->level > this->threshold_max) { VStruct->SetOff(Config->Get3WegePort()); }
    }
    if (this->Type != NONE && this->Type != EXTERN && mqtt) {
      if (this->raw > 0 )   { mqtt->Publish_Int((const char*)"raw", (int)this->raw, false); }
      if (this->level > 0 ) { mqtt->Publish_Int((const char*)"level", (int)this->level, false); }
    }
    
  #ifdef USE_OLED
    if (this->Type != NONE && this->Type != EXTERN) {
      if(this->oled) this->oled->SetLevel(this->level);
    }
  #endif
  
     if (this->Type != NONE && this->Type != EXTERN) {
      Config->logN(4, "measured sensor raw value: %d ", this->raw);
     }
  }
}

void sensor::LoadJsonConfig() {
  mqtt->ClearSubscriptions(MyMQTT::SENSOR);
  
  #ifdef USE_ADS1115
    this->ads1115_devices->clear();
  #endif

  String selection = "";

  if (configFS.exists("/config/sensorconfig.json")) {
    //file exists, reading and loading
    Config->logN(3, "reading sensorconfig.json file");
    File configFile = configFS.open("/config/sensorconfig.json", "r");
    if (configFile) {
      Config->logN(3, "sensorconfig.json is now open");
      ReadBufferingStream stream{configFile, 64};
      stream.find("\"data\":[");
      do {

        JsonDocument elem;
        DeserializationError error = deserializeJson(elem, stream); 
        if (error) {
          Config->logN(1, "Failed to parse sensorconfig.json data: %s, load default config", error.c_str()); 
        } else {
          // Print the result
          Config->logN(5, "parsing partial JSON of sensorconfig.json ok"); 
          Config->log(5, elem);
          
          if (elem["measurecycle"])         { this->measurecycle = _max(elem["measurecycle"].as<int>(), 10);}
          if (elem["measureDistMin"])       { this->measureDistMin = elem["measureDistMin"].as<int>();}
          if (elem["measureDistMax"])       { this->measureDistMax = elem["measureDistMax"].as<int>();}
          if (elem["pinhcsr04trigger"])     { this->pinTrigger = elem["pinhcsr04trigger"].as<int>() - 200;}
          if (elem["pinhcsr04echo"])        { this->pinEcho = elem["pinhcsr04echo"].as<int>() - 200;}
          if (elem["pinanalog"])            { this->pinAnalog = elem["pinanalog"].as<int>() - 200;}
          if (elem["threshold_min"])         { this->threshold_min = elem["threshold_min"].as<int>();}
          if (elem["threshold_max"])         { this->threshold_max = elem["threshold_max"].as<int>();}
          if (elem["ads1115_i2c"])          { this->ads1115_i2c = strtoul(elem["ads1115_i2c"].as<String>().c_str(), NULL, 16);} // hex convert to dec 
          if (elem["ads1115_port"])         { this->ads1115_port = elem["ads1115_port"].as<int>();}
          if (elem["externalSensor"])       { this->externalSensor = elem["externalSensor"].as<String>();}
          if (elem["selection"])            { selection = elem["selection"].as<String>(); }
          if (elem["sel_moisture"])         { if (elem["sel_moisture"].as<String>() == "on") {this->moistureEnabled = true;} else {this->moistureEnabled = false;}}

        #ifdef USE_ADS1115
          if (elem["mqtttopic"] && 
              elem["ads_addr"] &&
              elem["ads_port"]) {
                this->init_ads1115(strtoul(elem["ads_addr"].as<String>().c_str(), NULL, 16), elem["ads_port"].as<int>(), elem["mqtttopic"].as<String>());
              }
        #endif
        
        }
      } while (stream.findUntil(",","]"));

      if (selection == "analog")        { this->init_analog(this->pinAnalog); }
      else if (selection == "hcsr04")   { this->init_hcsr04(this->pinTrigger, this->pinEcho); }
      else if (selection == "extern")   { this->init_extern(this->externalSensor); }
      else if (selection == "none")     { this->setSensorType(NONE); Config->logN(3, "No LevelSensor requested"); }
            
      #ifdef USE_ADS1115  
        else if(selection == "ads1115") { this->setSensorType(ADS1115); this->init_ads1115(this->ads1115_i2c, this->ads1115_port); }
      #endif

    } else {
      Config->logN(3, "cannot open existing sensorconfig.json config File, load default SensorConfig"); // -> constructor
    }
  } else {
    Config->logN(3, "sensorconfig.json config File not exists, load default SensorConfig");
  }
}

void sensor::GetInitData(JsonDocument& json) {
  json["data"].to<JsonObject>();
  json["data"]["sel0"] = ((this->Type==NONE)?1:0);
  json["data"]["sel1"] = ((this->Type==HCSR04)?1:0);
  json["data"]["sel2"] = ((this->Type==ONBOARD_ANALOG)?1:0);

#ifdef USE_ADS1115
  std::ostringstream ads1115_i2c_hex;
  ads1115_i2c_hex << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)this->ads1115_i2c;
  
  json["data"]["sel3"] = ((this->Type==ADS1115)?1:0);
  json["data"]["ads1115_i2c"] = ads1115_i2c_hex.str(); 
  json["data"]["ads1115_port"] = this->ads1115_port;
  json["data"]["sel_moisture1"] = ((this->moistureEnabled)?0:1);
  json["data"]["sel_moisture2"] = ((this->moistureEnabled)?1:0);
  
  json["data"]["rows"].to<JsonArray>();
  
  uint8_t counter = 0;
  for (uint8_t i=0;i<this->ads1115_devices->size(); i++) {
    std::ostringstream moisture_i2c_hex;
    moisture_i2c_hex << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)this->ads1115_devices->at(i).i2cAddress;
    if (this->ads1115_devices->at(i).topic_chan1.length() > 0 &&
        !(this->GetType() == ADS1115  &&
         this->ads1115_i2c == this->ads1115_devices->at(i).i2cAddress &&
         this->ads1115_port == 0
        )) {
      json["data"]["rows"][counter]["mqtttopic"] = this->ads1115_devices->at(i).topic_chan1.c_str();
      json["data"]["rows"][counter]["ads_port"] = 0;
      json["data"]["rows"][counter]["ads_addr"] = moisture_i2c_hex.str(); 
      counter++;
    }

    if (this->ads1115_devices->at(i).topic_chan2.length() > 0 &&
        !(this->GetType() == ADS1115  &&
         this->ads1115_i2c == this->ads1115_devices->at(i).i2cAddress &&
         this->ads1115_port == 0
        )) {
      json["data"]["rows"][counter]["mqtttopic"] = this->ads1115_devices->at(i).topic_chan2.c_str();
      json["data"]["rows"][counter]["ads_port"] = 1;
      json["data"]["rows"][counter]["ads_addr"] = moisture_i2c_hex.str(); 
      counter++;
    }

    if (this->ads1115_devices->at(i).topic_chan3.length() > 0 &&
        !(this->GetType() == ADS1115  &&
         this->ads1115_i2c == this->ads1115_devices->at(i).i2cAddress &&
         this->ads1115_port == 0
        )) {
      json["data"]["rows"][counter]["mqtttopic"] = this->ads1115_devices->at(i).topic_chan3.c_str();
      json["data"]["rows"][counter]["ads_port"] = 2;
      json["data"]["rows"][counter]["ads_addr"] = moisture_i2c_hex.str(); 
      counter++;
    }

    if (this->ads1115_devices->at(i).topic_chan4.length() > 0 &&
        !(this->GetType() == ADS1115  &&
         this->ads1115_i2c == this->ads1115_devices->at(i).i2cAddress &&
         this->ads1115_port == 0
        )) {
      json["data"]["rows"][counter]["mqtttopic"] = this->ads1115_devices->at(i).topic_chan4.c_str();
      json["data"]["rows"][counter]["ads_port"] = 3;
      json["data"]["rows"][counter]["ads_addr"] = moisture_i2c_hex.str(); 
      counter++;
    }
  }

  // print template as first row
  if (counter == 0) {
    json["data"]["rows"][0]["mqtttopic"] = "moisture_1";
    json["data"]["rows"][0]["ads_port"] = 0;
    json["data"]["rows"][0]["ads_addr"] = "48";
  }

#else 
  json["data"]["sel_ADS1115"]["className"] = "hide";
  json["data"]["ads1115_0"]["className"] = "hide";
  json["data"]["ads1115_1"]["className"] = "hide";
  json["data"]["moistureTable"]["className"] = "hide";
#endif

  json["data"]["sel4"] = ((this->Type==EXTERN)?1:0);
  json["data"]["measurecycle"] = this->measurecycle;
  json["data"]["measureDistMin"] = this->measureDistMin;
  json["data"]["measureDistMax"] = this->measureDistMax;
  json["data"]["pinhcsr04trigger"] = this->pinTrigger + 200;
  json["data"]["pinhcsr04echo"] = this->pinEcho + 200;
  json["data"]["pinanalog"] = this->pinAnalog + 200;
  json["data"]["a_measureDistMin"] = this->measureDistMin;
  json["data"]["a_measureDistMax"] = this->measureDistMax;
  json["data"]["externalSensor"] = this->externalSensor;
  json["data"]["threshold_min"] = this->threshold_min;
  json["data"]["threshold_max"] = this->threshold_max;

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}
