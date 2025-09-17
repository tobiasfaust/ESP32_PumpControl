#include "doif.h"
#include <regex>

doIf::doIf(fs::LittleFSFS& configFS) : configFS(configFS) {
  _relationen = new std::vector<doIf_t>();
  this->LoadJsonConfig();
}

bool doIf::setActive(String topic, uint8_t port, bool active) {
  for (uint8_t i = 0; i < _relationen->size(); i++) {
    if (_relationen->at(i).TriggerTopic == topic && _relationen->at(i).ActorPort == port) {
      _relationen->at(i).enabled = active;
      return true;
    }
  }

  return false; 
}

void doIf::LoadJsonConfig() {
  _relationen->clear(); // leere den Vector bevor neu befuellt wird
  mqtt->ClearSubscriptions(MyMQTT::DOIF);
 
  bool loadDefaultConfig = false;
 
  if (configFS.exists("/doif.json")) {
    //file exists, reading and loading
    Config->logN(3, "reading doif.json file....");
    File configFile = configFS.open("/doif.json", "r");
    if (configFile) {
      Config->logN(3, "doif.json is now open");

      ReadBufferingStream stream{configFile, 64};
      stream.find("\"data\":[");
      do {
        JsonDocument elem;
        DeserializationError error = deserializeJson(elem, stream); 
 
        if (error) {
           loadDefaultConfig = true;
           Config->logN(1, "Failed to parse doif.json data: %s, load default config", error.c_str()); 
        } else {
           // Print the result
           Config->logN(3, "parsing JSON ok");
           Config->log(4, elem);
 
          if (elem["mqtttopic"] && elem["port"] && elem["port"] && elem["port"].as<int>() > 0) {
            doIf_t rel;
 
            rel.enabled = elem["active"].as<bool>();
            rel.TriggerTopic = elem["mqtttopic"].as<String>();
            rel.RegExpPattern = elem["pattern"].as<String>();
            rel.threshold = elem["threshold"].as<unsigned int>();
            rel.duration = elem["duration"].as<unsigned int>();
            rel.ActorPort = elem["port"].as<uint8_t>();

            _relationen->push_back(rel);
            mqtt->Subscribe(rel.TriggerTopic, MyMQTT::DOIF);
          } 
          
        }
      } while (stream.findUntil(",","]"));
    } else {
      loadDefaultConfig = true;
      Config->logN(1, "failed to load doif.json, load default config");
    }
  } else {
    loadDefaultConfig = true;
    Config->logN(3, "doif.json File not exists, load default config");
  }
   
  if (loadDefaultConfig) {
    Config->logN(3, "load doif DefaultConfig");
    doIf_t rel1;
    rel1.enabled = false;
    rel1.TriggerTopic = "flowercare/00:00:00:00:00:00";
    rel1.RegExpPattern = "moisture: (\\d+)";
    rel1.threshold = 30;
    rel1.duration = 60;
    rel1.ActorPort = 0;
    _relationen->push_back(rel1);

    doIf_t rel2;
    rel2.enabled = false;
    rel2.TriggerTopic = "home/MyDevice/sensor1";
    rel2.RegExpPattern = "(\\d+)";
    rel2.threshold = 30;
    rel2.duration = 60;
    rel2.ActorPort = 1;
    _relationen->push_back(rel2);
  }
  Config->logN(3, "%d DoIF relations are now loaded ", _relationen->size());

  _relationen->shrink_to_fit();
}

void doIf::GetInitData(JsonDocument& json) {
  
  JsonArray f = json["data"]["fc_relations"].to<JsonArray>();
  for (uint8_t i = 0; i < _relationen->size(); i++) {
    JsonObject o = f.add<JsonObject>();
    o["active"]["checked"] = _relationen->at(i).enabled;
    o["mqtttopic"] = _relationen->at(i).TriggerTopic;
    o["pattern"] = _relationen->at(i).RegExpPattern;
    o["ConfiguredPort"] = _relationen->at(i).ActorPort;
    o["threshold"] = _relationen->at(i).threshold;
    o["duration"] = _relationen->at(i).duration;
  }
  
  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";

}

void doIf::onMqttMessage(valveStructure* VStruct, String& topic, String& msg) {
  Config->logN(4, "DoIF MQTT Message received: %s", msg.c_str());

  //looking for the topic in the relations
  for (uint8_t i = 0; i < _relationen->size(); i++) {
    if (_relationen->at(i).TriggerTopic == topic && _relationen->at(i).enabled) {
      // found a matching relation
      // check if the msg matches the pattern
      std::regex pattern;
      try {
        pattern = std::regex(_relationen->at(i).RegExpPattern.c_str());
      } catch (const std::regex_error& e) {
        Config->logN(1, "Invalid regex pattern: %s", _relationen->at(i).RegExpPattern.c_str());
        return;
      }

      std::smatch matches;
      std::string msgStdStr = msg.c_str();
      if (std::regex_search(msgStdStr, matches, pattern)) {
        if (matches.size() > 1) { // first match is the whole string, second is the first group
          int moisture = atoi(matches[1].str().c_str());
          // if moisture is 0 then do nothing
          if (moisture == 0) {
            Config->logN(4, "%s: moisture: %d%% -> do nothing because moisture value is 0\n", topic.c_str(), moisture);
            return;
          }
          if (moisture < _relationen->at(i).threshold) {
            VStruct->OnForTimer(_relationen->at(i).ActorPort, _relationen->at(i).duration);
            Config->logN(3, "%s (moisture: %d%%) triggered valve %d for %d seconds", 
                topic.c_str(),
                moisture,
                _relationen->at(i).ActorPort,
                _relationen->at(i).duration);
          } else {
            Config->logN(4, "%s: moisture: %d%% -> above threshold of %d%%, do nothing\n",
                topic.c_str(),
                moisture,
                _relationen->at(i).threshold);
          }
        } else {
          Config->logN(2, "No capturing group found in regex pattern: %s", _relationen->at(i).RegExpPattern.c_str());
        }
      } else {
        Config->logN(4, "%s: message '%s' does not match pattern '%s', do nothing\n",
            topic.c_str(),
            msg.c_str(),
            _relationen->at(i).RegExpPattern.c_str());
      }
    }
  }
}