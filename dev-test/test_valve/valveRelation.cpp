#include "valveRelation.h"

valveRelation::valveRelation() {
  _relationen  = new std::vector<relation_t>{};
  _subscriber  = new std::vector<relation_t>{};

  LoadJsonConfig();
}

void valveRelation::AddRelation(bool enabled, String SubTopic, uint8_t Port) {
  relation_t rel;
  rel.enabled = enabled;
  rel.TriggerSubTopic = SubTopic;
  rel.ActorPort = Port;
  _relationen->push_back(rel); 
  Serial.print("Add Relation: "); Serial.print(rel.ActorPort);Serial.print(", "); Serial.println(rel.TriggerSubTopic);
}

void valveRelation::GetPortDependencies(std::vector<uint8_t>* Ports, String SubTopic) {
  char buffer[50] = {0};
  memset(buffer, 0, sizeof(buffer));
  
  for (uint8_t i=0; i<_relationen->size(); i++) {
    sprintf(buffer, "Deps: SubTopic: %s, Port: %d", _relationen->at(i).TriggerSubTopic.c_str(), _relationen->at(i).ActorPort); Serial.println(buffer);
    if (_relationen->at(i).TriggerSubTopic == SubTopic) {
      bool stillpresent=false;
      for (uint8_t j=0; j<Ports->size(); j++) {
        if (Ports->at(j) == _relationen->at(i).ActorPort) {stillpresent=true;}
      }
      if (!stillpresent) {Ports->push_back(_relationen->at(i).ActorPort); }
    }
  }
}

void valveRelation::AddSubscriberPort(uint8_t Port, String TriggerSubTopic) {
  char buffer[50] = {0};
  memset(buffer, 0, sizeof(buffer));
  relation_t s;
  s.enabled = true;
  s.TriggerSubTopic = TriggerSubTopic;
  s.ActorPort = Port;
  _subscriber->push_back(s); 
  sprintf(buffer, "Add Subscriber: %d - %s", _subscriber->at(0).ActorPort, _subscriber->at(0).TriggerSubTopic.c_str());
  Serial.println(buffer);   
}

void valveRelation::DelSubscriberPort(uint8_t Port) {
  for (auto i=_subscriber->begin(); i!=_subscriber->end(); ++i) {
     uint8_t p = std::distance(_subscriber->begin(), i);
     if (_subscriber->at(p).ActorPort == Port) { _subscriber->erase(i); Serial.print("Del Subscriber: "); Serial.println(Port);}
  }
}

uint8_t valveRelation::CountActiveSubscribers(String SubTopic) {
  uint8_t count=0;
  for (uint8_t i=0; i<_subscriber->size(); i++) {
    if (_subscriber->at(i).TriggerSubTopic == SubTopic) {count++;}
  }
  return count;
}

void valveRelation::StoreJsonConfig(String json) {
  //https://arduinojson.org/v5/api/jsonobject/begin_end/
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
    
  if (root.success()) {
    File configFile = SPIFFS.open("/Relations.json", "w");
    if (!configFile) {
      Serial.println("failed to open Relations.json file for writing");
    }
  
    root.printTo(Serial);
    root.printTo(configFile);
    configFile.close();

    LoadJsonConfig();
  }
}

void valveRelation::LoadJsonConfig() {
  bool loadDefaultConfig = false;
  char buffer[100] = {0};
  memset(buffer, 0, sizeof(buffer));

  _relationen->clear(); // leere den Valve Vector bevor neu befüllt wird
  _subscriber->clear();
  
  if (SPIFFS.exists("/Relations.json")) {
    File configFile = SPIFFS.open("/Relations.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success()) {
        uint8_t count = 0;
        if (json.containsKey("count")) { count = json["count"].as<int>(); }
        if(count == 0) {
          Serial.println("something went wrong with json config, load default config");
          loadDefaultConfig = true;
        }
        
        for (uint8_t i=0; i<count; i++) {
          bool enabled = false;
          String SubTopic = "";
          uint8_t Port = 0;
          
          sprintf(buffer, "active_%d", i);
          if (json[buffer] && json[buffer] == 1) {enabled = true;} else {enabled = false;}
            
          sprintf(buffer, "mqtttopic_%d", i);
          if (json.containsKey(buffer)) {SubTopic = json[buffer].as<String>();}
 
          sprintf(buffer, "port_%d_1", i);
          if (json.containsKey(buffer) && json[buffer].as<int>() > 0) { Port = json[buffer].as<int>();}

          AddRelation(enabled, SubTopic, Port);
        }
        
      } else {
        loadDefaultConfig = true;
      }
    } else {
      loadDefaultConfig = true;
    }
  } else {
    loadDefaultConfig = true;
  }

  if (loadDefaultConfig) {
    Serial.println("lade Relation DefaultConfig");
    AddRelation(true, "testhost/TestVirtualValve1", 203);
    AddRelation(true, "testhost/TestVirtualValve2", 203);
  }
}

void valveRelation::GetWebContent(String* html) {
  char buffer[200] = {0};
  memset(buffer, 0, sizeof(buffer));

    html->concat("<p><input type='button' value='&#10010; add new Port' onclick='addrow()'></p>\n");
  html->concat("<form id='submitForm'>\n");
  html->concat("<table id='maintable' class='editorDemoTable'>\n");
  html->concat("<thead>\n");
  html->concat("<tr>\n");
  html->concat("<td style='width: 25px;'>Nr</td>\n");
  html->concat("<td style='width: 25px;'>Active</td>\n");
  html->concat("<td style='width: 250px;'>Trigger Topic</td>\n");
  html->concat("<td style='width: 250px;'>Port</td>\n");
  html->concat("<td style='width: 25px;'>Delete</td>\n");

  html->concat("</tr>\n");
  html->concat("</thead>\n");
  html->concat("<tbody>\n\n");

  for (uint8_t i=0; i<_relationen->size(); i++) {
    html->concat("<tr>\n");
    sprintf(buffer, "  <td>%d</td>\n", i+1);
    html->concat(buffer);
    html->concat("  <td>\n");
    html->concat("    <div class='onoffswitch'>");
    sprintf(buffer, "      <input type='checkbox' name='active_%d' class='onoffswitch-checkbox' id='myonoffswitch_%d' %s>\n", i, i, (_relationen->at(i).enabled?"checked":""));
    html->concat(buffer);
    sprintf(buffer, "      <label class='onoffswitch-label' for='myonoffswitch_%d'>\n", i);
    html->concat(buffer);
    html->concat("        <span class='onoffswitch-inner'></span>\n");
    html->concat("        <span class='onoffswitch-switch'></span>\n");
    html->concat("      </label>\n");
    html->concat("    </div>\n");
    html->concat("  </td>\n");

    sprintf(buffer, "  <td><input id='mqtttopic_%d' name='mqtttopic_%d' type='text' size='10' value='%s'/></td>\n", i, i, _relationen->at(i).TriggerSubTopic.c_str());
    html->concat(buffer);
    sprintf(buffer, "  <td><input id='ConfiguredTopics_%d' name='port_%d' type='text' size='10' value='%s'/></td>\n", i, i, _relationen->at(i).ActorPort);
    html->concat(buffer);
    html->concat("  <td><input type='button' value='&#10008;' onclick='delrow(this)'></td>\n");
    html->concat("</tr>\n");
  }

  html->concat("</tbody>\n");
  html->concat("</table>\n");
  html->concat("</form>\n\n<br />\n");
  html->concat("<form id='jsonform' action='StoreRelations' method='POST' onsubmit='return onSubmit()'>\n");
  html->concat("  <input type='text' id='json' name='json' />\n");
  html->concat("  <input type='submit' value='Speichern' />\n");
  html->concat("</form>\n\n");
  html->concat("<div id='ErrorText' class='errortext'></div>\n");
}

