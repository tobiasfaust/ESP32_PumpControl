#include "mqtt.h"

MyMQTT::MyMQTT(AsyncWebServer* server, DNSServer* dns, const char* MqttServer, uint16_t port, String basepath, String root): MQTT(server, dns, MqttServer, port, basepath, root) {

}
void MyMQTT::SetOled(OLED* oled) {
  this->oled=oled;  
}

void MyMQTT::loop() {
  MQTT::loop();
  if (WiFi.status() == WL_CONNECTED) {
    if(this->oled) {
      this->oled->SetIP(WiFi.localIP().toString());
      this->oled->SetRSSI(WiFi.RSSI());
      this->oled->SetSSID(WiFi.SSID());
      this->oled->SetWiFiConnected(true);
    }
  } else {
    if(this->oled) this->oled->SetWiFiConnected(false);
  }
}

void MyMQTT::reconnect() {
  MQTT::reconnect();
  if(this->oled) this->oled->SetMqttConnected(MQTT::GetConnectStatusMqtt());
}





MQTT::MQTT(AsyncWebServer* server, DNSServer* dns, const char* MqttServer, uint16_t port, String basepath, String root): server(server), dns(dns) { 
  this->mqtt_basepath = basepath;
  this->mqtt_root = root;
  
  this->subscriptions = new std::vector<subscription_t>{};

  espClient = WiFiClient();
  WiFi.mode(WIFI_STA); 
  this->mqtt = new PubSubClient();
  
  this->wifiManager = new AsyncWiFiManager(server, dns);

  if (Config->GetDebugLevel() >=4) wifiManager->setDebugOutput(true); 
    else wifiManager->setDebugOutput(false); 

  wifiManager->setConnectTimeout(60);
  wifiManager->setConfigPortalTimeout(300);
  WiFi.setHostname(mqtt_root.c_str());
  Serial.println("WiFi Start");
  
  if (!wifiManager->autoConnect(("AP_" + mqtt_root).c_str(), "password") ) {
    Serial.println("failed to connect and start configPortal");
    wifiManager->startConfigPortal(("AP_" + mqtt_root).c_str(), "password");
  }
  
  Serial.print("WiFi connected with local IP: ");
  Serial.println(WiFi.localIP());
  //WiFi.printDiag(Serial);

  Serial.print("Starting MQTT ("); Serial.print(MqttServer); Serial.print(":");Serial.print(port);Serial.println(")");
  this->mqtt->setClient(espClient);
  this->mqtt->setServer(MqttServer, port);
  this->mqtt->setCallback([this] (char* topic, byte* payload, unsigned int length) { this->callback(topic, payload, length); });
}

void MQTT::reconnect() {
  char topic[50];
  char LWT[50];
  memset(&LWT[0], 0, sizeof(LWT));
  memset(&topic[0], 0, sizeof(topic));
  
  if (Config->UseRandomMQTTClientID()) { 
    snprintf (topic, sizeof(topic), "%s-%s", this->mqtt_root.c_str(), String(random(0xffff)).c_str());
  } else {
    snprintf (topic, sizeof(topic), "%s-%08X", this->mqtt_root.c_str(), WIFI_getChipId());
  }
  snprintf(LWT, sizeof(LWT), "%s/state", this->mqtt_root.c_str());
  
  Serial.printf("Attempting MQTT connection as %s \n", topic);
  
  if (this->mqtt->connect(topic, Config->GetMqttUsername().c_str(), Config->GetMqttPassword().c_str(), LWT, true, false, "Offline")) {
    Serial.println("connected... ");
    // Once connected, publish basics ...
    this->Publish_IP();
    this->Publish_String("state", "Online", false); //LWT reset
    
    // ... and resubscribe if needed
    for (uint8_t i=0; i< this->subscriptions->size(); i++) {
      if (this->subscriptions->at(i).active == true) {
        this->mqtt->subscribe(this->subscriptions->at(i).subscription.c_str()); 
        Serial.print(F("MQTT Subscribed to: ")); Serial.println(FPSTR(this->subscriptions->at(i).subscription.c_str()));
      }
    }

  } else {
    Serial.print(F("failed, rc="));
    Serial.print(this->mqtt->state());
    Serial.println(F(" try again in few seconds"));
  }
}

void MQTT::disconnect() {
  this->mqtt->disconnect();
}

void MQTT::callback(char* topic, byte* payload, unsigned int length) {
  if (MyCallback) {
    MyCallback(topic,payload,length);
  }
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
  
  if (this->mqtt->connected()) {
    this->mqtt->publish((const char*)topic.c_str(), value.c_str(), true);
    if (Config->GetDebugLevel() >=3) {
      Serial.printf("Publish %s: %s \n", topic.c_str(), value.c_str());
    }
  } else { if (Config->GetDebugLevel() >=2) {Serial.println(F("Request for MQTT Publish, but not connected to Broker")); }}
}

String MQTT::getTopic(String subtopic, bool fulltopic) {
  if (!fulltopic) {
      subtopic = this->mqtt_basepath + "/" + this->mqtt_root +  "/" + subtopic;
  }
  return subtopic;
}

void MQTT::Publish_IP() {
  char buffer[15] = {0};
  memset(&buffer[0], 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", WiFi.localIP().toString().c_str());
  Publish_String("IP", buffer, false);
}

void MQTT::setCallback(CALLBACK_FUNCTION) {
    this->MyCallback = MyCallback;
}

/*******************************************************
 * subscribe to a special topic (without /# at end)
*******************************************************/
void MQTT::Subscribe(String topic, MqttSubscriptionType_t identifier) {
  char buffer[100] = {0};
  memset(buffer, 0, sizeof(buffer));
  subscription_t sub = {};
  snprintf(buffer, sizeof(buffer), "%s/#", topic.c_str());
  sub.subscription = buffer;
  sub.identifier = identifier;
  sub.active = true;
  this->subscriptions->push_back(sub);
  if (mqtt->connected()) {mqtt->subscribe(sub.subscription.c_str()); Serial.print(F("MQTT Subscribed to: ")); Serial.println(FPSTR(sub.subscription.c_str()));}
}

void MQTT::ClearSubscriptions(MqttSubscriptionType_t identifier) {
  //TODO: memory leak? besser komplett löschen und neu aufsetzen, siehe valveRelation::DelSubscriber()
  for ( uint8_t i=0; i< this->subscriptions->size(); i++) {
    if (mqtt->connected() && this->subscriptions->at(i).active == true && this->subscriptions->at(i).identifier == identifier) { 
      this->mqtt->unsubscribe(this->subscriptions->at(i).subscription.c_str()); 
    }
    if (this->subscriptions->at(i).identifier == identifier) { 
      this->subscriptions->at(i).active = false;
    }
  }
}

void MQTT::loop() {
  if (this->mqtt_root != Config->GetMqttRoot()) {
    if (Config->GetDebugLevel() >=3) {
      Serial.printf("MQTT DeviceName has changed via Web Configuration from %s to %s \n", this->mqtt_root.c_str(), Config->GetMqttRoot().c_str());
      Serial.println(F("Initiate Reconnect"));
    }
    this->mqtt_root = Config->GetMqttRoot();
    if (this->mqtt->connected()) this->mqtt->disconnect();
  }

  if (this->mqtt_basepath != Config->GetMqttBasePath()) {
    if (Config->GetDebugLevel() > 3) {
      Serial.printf("MQTT Basepath has changed via Web Configuration from %s to %s \n", this->mqtt_basepath.c_str(), Config->GetMqttBasePath().c_str());
      Serial.println(F("Initiate Reconnect"));
    }
    this->mqtt_basepath = Config->GetMqttBasePath();
    if (this->mqtt->connected()) this->mqtt->disconnect();
  }

  // WIFI lost, try to reconnect
  if (WiFi.status() != WL_CONNECTED) {
    if (Config->GetDebugLevel() > 1) {
      Serial.println("WIFI lost, try to reconnect...");
    }
    wifiManager->setConfigPortalTimeout(0);
    while (WiFi.status() != WL_CONNECTED) {
      delay(5000);
      WiFi.begin();        
    }
  }
  
  // WIFI ok, MQTT lost
  if (!this->mqtt->connected() && WiFi.status() == WL_CONNECTED) { 
    if (millis() - mqttreconnect_lasttry > 10000) {
      espClient = WiFiClient();
      this->reconnect(); 
      this->mqttreconnect_lasttry = millis();
    }
  } else if (WiFi.status() == WL_CONNECTED) { 
    this->mqtt->loop();
  }

  if (WiFi.status() == WL_CONNECTED) {
    this->ConnectStatusWifi = true;
  } else {
    this->ConnectStatusWifi = false;
  }

  if (this->mqtt->connected()) {
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
