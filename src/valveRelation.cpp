#include "valveRelation.h"

valveRelation::valveRelation(fs::LittleFSFS& configFS) :
  configFS(configFS) {
  _relationen  = new std::vector<relation_t>{};
  _subscriber  = new std::vector<subscriber_t>{};
  LoadJsonConfig();
}

void valveRelation::AddRelation(bool enabled, String TriggerTopic, uint8_t Port, bool EnableByBypass) {
  relation_t rel;
  rel.enabled = enabled;
  rel.TriggerTopic = TriggerTopic;
  rel.ActorPort = Port;
  rel.EnableByBypass = EnableByBypass;
  _relationen->push_back(rel);
  if (enabled) { 
    mqtt->Subscribe(TriggerTopic, MyMQTT::RELATION); 
  }
}

void valveRelation::GetPortDependencies(std::vector<uint8_t>* Ports, String TriggerTopic) {
  for (uint8_t i=0; i<_relationen->size(); i++) {
    if (_relationen->at(i).TriggerTopic == TriggerTopic && _relationen->at(i).enabled) {
      bool stillpresent=false;
      for (uint8_t j=0; j<Ports->size(); j++) {
        if (Ports->at(j) == _relationen->at(i).ActorPort) {stillpresent=true;}
      }
      if (!stillpresent) {Ports->push_back(_relationen->at(i).ActorPort); }
    }
  }
}

/****************************************************************************
* prueft ob die Relation das Bypass Flag gesetzt hat
*****************************************************************************/
bool valveRelation::CheckEnabledByBypass(uint8_t ActorPort, String TriggerTopic) {
  for (uint8_t i=0; i<_relationen->size(); i++) {
    if (_relationen->at(i).TriggerTopic == TriggerTopic && _relationen->at(i).ActorPort == ActorPort && _relationen->at(i).EnableByBypass) {
      return true;
    }
  }
  return false;
}

/*aufgrund einer Relation wurde ein Ventil geöffnet, merken in _subscriber */
void valveRelation::AddSubscriber(uint8_t ActorPort, String TriggerTopic) {
  subscriber_t s;
  s.TriggerTopic = TriggerTopic;
  s.ActorPort = ActorPort;
  _subscriber->push_back(s); 
}

/*aufgrund einer Relation wurde ein Ventil wieder geschlossen, löschen aus  _subscriber */
void valveRelation::DelSubscriber(String TriggerTopic) {
  //lösche TriggerTopic auf dem ActorPort
  std::vector<subscriber_t> t;
  for (uint8_t i=0; i<_subscriber->size(); i++) {
    if (_subscriber->at(i).TriggerTopic != TriggerTopic) { t.push_back(_subscriber->at(i)); }
  }
  _subscriber->clear();
  
  for (uint8_t i=0; i<t.size(); i++) {
    _subscriber->push_back(t.at(i));
  }

  _subscriber->shrink_to_fit();
}

uint8_t valveRelation::CountActiveSubscribers(uint8_t ActorPort) {
  uint8_t count=0;
  for (uint8_t i=0; i<_subscriber->size(); i++) {
    if (_subscriber->at(i).ActorPort == ActorPort) {count++;}
  }
  return count;
}

void valveRelation::LoadJsonConfig() {
  _relationen->clear(); // leere den Valve Vector bevor neu befüllt wird
  _subscriber->clear();
  mqtt->ClearSubscriptions(MyMQTT::RELATION);

  bool loadDefaultConfig = false;


  if (configFS.exists("/config/relations.json")) {
    //file exists, reading and loading
    Config->logN(3, "reading relations.json file....");
    File configFile = configFS.open("/config/relations.json", "r");
    if (configFile) {
      Config->logN(3, "relations.json is now open");

      ReadBufferingStream stream{configFile, 64};
      stream.find("\"data\":[");
      do {
        JsonDocument elem;
        DeserializationError error = deserializeJson(elem, stream); 

        if (error) {
          loadDefaultConfig = true;
          Config->logN(1, "Failed to parse relations.json data: %s, load default config", error.c_str()); 
        } else {
          // Print the result
          Config->logN(4, "parsing JSON ok");
          Config->log(5, elem);

          bool enabled = false;
          bool EnableByBypass = false;
          String SubTopic = (char*)0;
          uint8_t Port = 0;

          if (elem["active"] && elem["active"].as<bool>()) {enabled = elem["active"].as<bool>();} else {enabled = false;}
          if (elem["mqtttopic"]) {SubTopic = elem["mqtttopic"].as<String>();}
          if (elem["port"] && elem["port"].as<int>() > 0) { Port = elem["port"].as<int>();}
          if (elem["EnableByBypass"] && elem["EnableByBypass"].as<bool>()) {EnableByBypass = elem["EnableByBypass"].as<bool>();} else {EnableByBypass = false;}

          this->AddRelation(enabled, SubTopic, Port, EnableByBypass);
        }

      } while (stream.findUntil(",","]"));
    } else {
      loadDefaultConfig = true;
      Config->logN(1, "failed to load relations.json, load default config");
    }
  } else {
    loadDefaultConfig = true;
    Config->logN(3, "relations.json File not exists, load default config");
  }
  
  if (loadDefaultConfig) {
    Config->logN(3, "load Relations DefaultConfig");
    this->AddRelation(false, "testhost/TestValve1", 203, false);
    this->AddRelation(false, "testhost/TestValve2", 204, false);
  }
  Config->logN(3, "%d relations are now loaded ", _relationen->size());

  _relationen->shrink_to_fit();
}

void valveRelation::GetInitData(JsonDocument& json) {
  json["data"].to<JsonObject>();
  JsonArray row = json["data"]["rows"].to<JsonArray>();

  for (uint8_t i=0; i< _relationen->size(); i++) {
    row[i]["active"] = (_relationen->at(i).enabled?1:0);
    row[i]["mqtttopic"] = _relationen->at(i).TriggerTopic;
    row[i]["ConfiguredPort"] = _relationen->at(i).ActorPort;
    row[i]["EnableByBypass"] = (_relationen->at(i).EnableByBypass?1:0);
  }

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

