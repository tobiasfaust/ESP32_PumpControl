#include "mqtt.h"

MQTT::MQTT(AsyncWebServer* server, DNSServer *dns, const char* MqttServer, uint16_t MqttPort, String MqttBasepath, String MqttRoot, char* APName, char* APpassword): 
server(server), dns(dns), mqtt_root(MqttRoot), mqtt_basepath(MqttBasepath), ConnectStatusWifi(false), ConnectStatusMqtt(false) { 
  
  this->subscriptions = new std::vector<String>{};
 
//  this->mqtt = new PubSubClient();
  WiFi.setHostname(this->mqtt_root.c_str());

#ifdef ESP32  
  WiFi.onEvent(std::bind(&MQTT::WifiOnEvent, this, std::placeholders::_1));
#endif

  if (Config->GetDebugLevel() >=3) {
    WebSerial.printf("Go into %s Mode\n", (Config->GetUseETH()?"ETH":"Wifi"));
  }

  
  if (Config->GetUseETH()) {
    #ifdef ESP32
      //ETH.begin(1, 16, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO0_IN);
      eth_shield_t* shield = this->GetEthShield(Config->GetLANBoard());
      ETH.begin(shield->PHY_ADDR,
                shield->PHY_POWER,
                shield->PHY_MDC,
                shield->PHY_MDIO,
                shield->PHY_TYPE,
                shield->CLK_MODE);
    #endif

    this->WaitForConnect();
  } else {

    WiFi.mode(WIFI_STA);
    this->wifiManager = new AsyncWiFiManager(server, dns);

    if (Config->GetDebugLevel() >=4) wifiManager->setDebugOutput(true); 
      else wifiManager->setDebugOutput(false); 

    wifiManager->setConnectTimeout(60);
    wifiManager->setConfigPortalTimeout(300);
    WebSerial.println("WiFi Start");
    
  //  wifiManager->startConfigPortal("OnDemandAP");

    if (!wifiManager->autoConnect(APName, APpassword) ) {
      WebSerial.println("failed to connect and start configPortal");
      wifiManager->startConfigPortal(APName, APpassword);
    }
  }
  
  //WiFi.printDiag(Serial);

  WebSerial.printf("Starting MQTT (%s:%d)\n", MqttServer, MqttPort);
  espClient = WiFiClient();
  
  PubSubClient::setClient(espClient);
  PubSubClient::setServer(MqttServer, MqttPort);
}

#ifdef ESP32
void MQTT::WifiOnEvent(WiFiEvent_t event) {
    if (Config->GetDebugLevel()>=4) {WebSerial.printf("[WiFi-event] event: %d\n", event);}

    switch (event) {
        case ARDUINO_EVENT_WIFI_READY: 
            WebSerial.println("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            WebSerial.println("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            WebSerial.println("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            WebSerial.println("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            WebSerial.println("Connected to access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            WebSerial.println("Disconnected from WiFi access point");
            this->ConnectStatusWifi = false;
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            WebSerial.println("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            WebSerial.printf("WiFi connected with local IP: %s\n", WiFi.localIP().toString().c_str());
            this->ipadresse = WiFi.localIP();
            this->ConnectStatusWifi = true;
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            WebSerial.println("Lost IP address and IP address is reset to 0");
            this->ConnectStatusWifi = false;
            this->ipadresse = (0,0,0,0);
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            WebSerial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            WebSerial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            WebSerial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            WebSerial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            WebSerial.println("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            WebSerial.println("WiFi access point  stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            WebSerial.println("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            WebSerial.println("Client disconnected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            WebSerial.println("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            WebSerial.println("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            WebSerial.println("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            WebSerial.println("STA IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP6:
            WebSerial.println("Ethernet IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_START:
            WebSerial.println("Ethernet started");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            WebSerial.println("Ethernet stopped");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            WebSerial.println("Ethernet connected");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            WebSerial.println("Ethernet disconnected");
            this->ConnectStatusWifi = false;
            this->ipadresse = (0,0,0,0);
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            if (!this->ConnectStatusWifi) {
              WebSerial.printf("ETH MAC: %s, IPv4: %s, %s, Mbps: %d\n", 
                ETH.macAddress().c_str(), 
                ETH.localIP().toString().c_str(),
                (ETH.fullDuplex()?"FULL_DUPLEX":"HALF_DUPLEX"),
                ETH.linkSpeed()
              );
              this->ipadresse = ETH.localIP();
              this->ConnectStatusWifi = true;
            }
            break;
        default: break;
    }
}
#endif

/*######################################
return LanShield parameter tuple 
########################################*/
eth_shield_t* MQTT::GetEthShield(String ShieldName) {
  for(uint8_t i=0; i<this->lan_shields.size(); i++) {
    if(this->lan_shields.at(i).name == ShieldName) {
      return &this->lan_shields.at(i);
      break;
    }
  }
  return &this->lan_shields.at(0);
}

void MQTT::WaitForConnect() {
  while (!this->ConnectStatusWifi)
    delay(100);
WebSerial.println("Wait for connect");    
    //yield();
}

void MQTT::reconnect() {
  char topic[50];
  char LWT[50];
  memset(&LWT[0], 0, sizeof(LWT));
  memset(&topic[0], 0, sizeof(topic));
  
  if (Config->UseRandomMQTTClientID()) { 
    snprintf (topic, sizeof(topic), "%s-%s", this->mqtt_root.c_str(), String(random(0xffff)).c_str());
  } else {
    snprintf (topic, sizeof(topic), "%s-%08X", this->mqtt_root.c_str(), ESP_getChipId());
  }
  snprintf(LWT, sizeof(LWT), "%s/state", this->mqtt_root.c_str());
  
  WebSerial.printf("Attempting MQTT connection as %s \n", topic);
  
  if (PubSubClient::connect(topic, Config->GetMqttUsername().c_str(), Config->GetMqttPassword().c_str(), LWT, true, false, "Offline")) {
    WebSerial.println("connected... ");
    // Once connected, publish basics ...
    this->Publish_IP();
    this->Publish_String("version", Config->GetReleaseName(), false);
    this->Publish_String("state", "Online", false); //LWT reset
    
    // ... and resubscribe if needed
    for (uint8_t i=0; i< this->subscriptions->size(); i++) {
      PubSubClient::subscribe(this->subscriptions->at(i).c_str()); 
      WebSerial.print("MQTT resubscribed to: "); WebSerial.println(this->subscriptions->at(i).c_str());
    }

  } else {
    WebSerial.print(F("failed, rc="));
    WebSerial.print(PubSubClient::state());
    WebSerial.println(F(" try again in few seconds"));
  }
}

void MQTT::disconnect() {
  PubSubClient::disconnect();
}

void MQTT::Publish_Bool(const char* subtopic, bool b, bool fulltopic) {
  String s;
  if(b) {s = "1";} else {s = "0";};
  Publish_String(subtopic, s, fulltopic);
}

void MQTT::Publish_Int(const char* subtopic, int number, bool fulltopic) {
  char buffer[20] = {0}; 
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%d", number);
  Publish_String(subtopic, (String)buffer, fulltopic);
}

void MQTT::Publish_Float(const char* subtopic, float number, bool fulltopic) {
  char buffer[10] = {0};
  memset(&buffer[0], 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%.2f", number);
  Publish_String(subtopic, (String)buffer, fulltopic);
}

void MQTT::Publish_String(const char* subtopic, String value, bool fulltopic) {
  String topic = this->getTopic(String(subtopic), fulltopic);

  if (PubSubClient::connected()) {
    PubSubClient::publish((const char*)topic.c_str(), value.c_str(), true);
    if (Config->GetDebugLevel() >=3) {
      WebSerial.printf("Publish %s: %s \n", topic.c_str(), value.c_str());
    }
  } else { if (Config->GetDebugLevel() >=2) {WebSerial.println(F("Request for MQTT Publish, but not connected to Broker")); }}
}

String MQTT::getTopic(String subtopic, bool fulltopic) {
  if (!fulltopic) {
      subtopic = this->mqtt_basepath + "/" + this->mqtt_root +  "/" + subtopic;
  }
  return subtopic;

}

void MQTT::Publish_IP() { 
  char buffer[16] = {0};
  memset(&buffer[0], 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", this->ipadresse.toString().c_str());
  Publish_String("IP", buffer, false);
}

/*******************************************************
 * subscribe to a special topic (without /# at end)
*******************************************************/
void MQTT::Subscribe(String topic) {
  this->subscriptions->push_back(topic);
  if (PubSubClient::connected()) {
    PubSubClient::subscribe(topic.c_str());
    if (Config->GetDebugLevel() >=3) {
      WebSerial.printf("MQTT now subscribed to: %s\n", topic.c_str());
    }
  }
}

void MQTT::ClearSubscriptions() {
  for ( uint8_t i=0; i< this->subscriptions->size(); i++) {
    if (PubSubClient::connected()) { 
      PubSubClient::unsubscribe(this->subscriptions->at(i).c_str()); 
    }
  }
  this->subscriptions->clear();
  this->subscriptions->shrink_to_fit();
}

void MQTT::loop() { 

  #ifdef ESP8266
    if (WiFi.status() == WL_CONNECTED) {
      this->ConnectStatusWifi = true;
      this->ipadresse = WiFi.localIP();
    } else {
      this->ConnectStatusWifi = false;
      this->ipadresse = (0,0,0,0);
    }
  #endif

  if (this->mqtt_root != Config->GetMqttRoot()) {
    if (Config->GetDebugLevel() >=3) {
      WebSerial.printf("MQTT DeviceName has changed via Web Configuration from %s to %s \n", this->mqtt_root.c_str(), Config->GetMqttRoot().c_str());
      WebSerial.println(F("Initiate Reconnect"));
    }
    this->mqtt_root = Config->GetMqttRoot();
    if (PubSubClient::connected()) PubSubClient::disconnect();
  }

  if (this->mqtt_basepath != Config->GetMqttBasePath()) {
    if (Config->GetDebugLevel() > 3) {
      WebSerial.printf("MQTT Basepath has changed via Web Configuration from %s to %s \n", this->mqtt_basepath.c_str(), Config->GetMqttBasePath().c_str());
      WebSerial.println(F("Initiate Reconnect"));
    }
    this->mqtt_basepath = Config->GetMqttBasePath();
    if (PubSubClient::connected()) PubSubClient::disconnect();
  }
//WebSerial.println("Checking Loop MQTT: ETH");
  // WIFI lost, try to reconnect
  if (!Config->GetUseETH() && !this->ConnectStatusWifi) {
WebSerial.println("WIFI lost, try to reconnect");    
    wifiManager->setConfigPortalTimeout(0);
    WiFi.begin();        
  }
//WebSerial.println("Checking Loop MQTT: WIFI ok");
  // WIFI ok, MQTT lost
  if (!PubSubClient::connected() && this->ConnectStatusWifi) { 
    if (millis() - mqttreconnect_lasttry > 10000) {
      //espClient = WiFiClient();
      this->reconnect(); 
      this->mqttreconnect_lasttry = millis();
    }
  } else if (this->ConnectStatusWifi) {     
    PubSubClient::loop();
  }

  if (PubSubClient::connected()) {
    this->ConnectStatusMqtt = true;
  } else {
    this->ConnectStatusMqtt = false;
  }

  if (Config->GetDebugLevel() >=4 && millis() - this->last_keepalive > (30 * 1000))  {
    this->last_keepalive = millis();
    
    if (Config->GetDebugLevel() >=4) {
      char buffer[100] = {0};
      memset(buffer, 0, sizeof(buffer));
      
      snprintf(buffer, sizeof(buffer), "%d", ESP.getFreeHeap());
      this->Publish_String("memory", buffer, false);

      snprintf(buffer, sizeof(buffer), "%d", WiFi.RSSI());
      this->Publish_String("rssi", buffer, false);
    }
  }
  
}
