#include "flowcontrol.h"
#include <map>

std::vector<flowControl::flowcontrol_t> flowControl::flowControlItems;

flowControl::flowControl(fs::LittleFSFS& configFS) :
  configFS(configFS),
  lastCalculationTime(0) {
  
  LoadJsonConfig();
}

void flowControl::onValues(std::function<void(JsonDocument&)> callback) {
    this->onValuesCallback = callback;
}

void flowControl::AddFlowControl(bool enabled, String name, uint8_t port, unsigned int ImpPerLitre) {
  flowcontrol_t flow;
  flow.enabled = enabled;
  flow.name = name;
  flow.port = port;
  flow.ImpPerLitre = ImpPerLitre;
  flowControlItems.push_back(flow);

  if (enabled) {
    //register port as INPUT_PULLUP
    pinMode(flow.port - 200, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(flow.port - 200), isrHandler, (void*)(flow.port - 200), FALLING);

    Config->logN(4, "FlowControl '%s' (Port: %d) added", flow.name.c_str(), flow.port);
  }
}

void flowControl::DelFlowControl() {
  for (uint8_t i=0; i< flowControlItems.size(); i++) {
    pinMode(flowControlItems.at(i).port - 200, INPUT);
    detachInterrupt(digitalPinToInterrupt(flowControlItems.at(i).port - 200));
  }
  Config->logN(4, "All FlowControl items removed");
  flowControlItems.clear();
  flowControlItems.shrink_to_fit();
}

void flowControl::DelFlowControl(uint8_t port) {
  for (uint8_t i=0; i< flowControlItems.size(); i++) {
    if (flowControlItems.at(i).port == port) {
      flowControlItems.erase(flowControlItems.begin() + i);
      Config->logN(4, "FlowControl '%s' (Port: %d) removed", flowControlItems.at(i).name.c_str(), port);
      break;
    }
  }

  // unregister port, set to output mode
  pinMode(port - 200, OUTPUT);
  detachInterrupt(digitalPinToInterrupt(port - 200));

  flowControlItems.shrink_to_fit();
}

int flowControl::findIndexByGpio(gpio_num_t gpio) {
    for (size_t i = 0; i < flowControlItems.size(); i++) {
        if (flowControlItems.at(i).port == gpio + 200) return i;
    }
    return -1;
}

int flowControl::findIndexByName(const String& name) {
    for (size_t i = 0; i < flowControlItems.size(); i++) {
        if (flowControlItems.at(i).name == name) return i;
    }
    return -1;
}

void IRAM_ATTR flowControl::isrHandler(void* arg) {
    int gpio = (int)arg;
    int idx = findIndexByGpio((gpio_num_t)gpio);
    if (idx >= 0 && flowControlItems.at(idx).enabled) {
      flowControlItems.at(idx).count++;
    }
}

void flowControl::LoadJsonConfig() {
  this->DelFlowControl(); // leere den Valve Vector bevor neu befüllt wird
  bool loadDefaultConfig = false;


  if (configFS.exists("/flowcontrol.json")) {
    //file exists, reading and loading
    Config->logN(3, "reading flowcontrol.json file....");
    File configFile = configFS.open("/flowcontrol.json", "r");
    if (configFile) {
      Config->logN(4, "flowcontrol.json is now open");

      ReadBufferingStream stream{configFile, 64};
      stream.find("\"data\":[");
      do {
        JsonDocument elem;
        DeserializationError error = deserializeJson(elem, stream); 

        if (error) {
          loadDefaultConfig = true;
          Config->logN(1, "Failed to parse flowcontrol.json data: %s, load default config", error.c_str()); 
        } else {
          // Print the result
          Config->logN(4, "parsing JSON ok");
          Config->log(5, elem);

          bool enabled = false;
          String name = (char*)0;
          uint8_t port = 0;
          unsigned int ImpPerLitre = 0;

          if (elem["active"] && elem["active"].as<bool>()) {enabled = elem["active"].as<bool>();} else {enabled = false;}
          if (elem["name"]) {name = elem["name"].as<String>();}
          if (elem["port"] && elem["port"].as<int>() > 0) { port = elem["port"].as<int>();}
          if (elem["ImpPerLitre"] && elem["ImpPerLitre"].as<int>() > 0) { ImpPerLitre = elem["ImpPerLitre"].as<int>();}

          this->AddFlowControl(enabled, name, port, ImpPerLitre);
        }

      } while (stream.findUntil(",","]"));
    } else {
      loadDefaultConfig = true;
      Config->logN(1, "failed to load flowcontrol.json, load default config");
    }
  } else {
    loadDefaultConfig = true;
    Config->logN(3, "flowcontrol.json File not exists, load default config");
  }
  
  if (loadDefaultConfig) {
    Config->logN(4, "load FlowControl DefaultConfig now");
    this->AddFlowControl(false, "flowcontrol1", 232, 0);
    this->AddFlowControl(false, "flowcontrol2", 233, 0);
  }
  Config->logN(3, "%d flowControl devices are now loaded ", flowControlItems.size());

  flowControlItems.shrink_to_fit();
}

void flowControl::GetInitData(JsonDocument& json) {
  json["data"].to<JsonObject>();
  JsonArray row = json["data"]["rows"].to<JsonArray>();

  for (uint8_t i=0; i< flowControlItems.size(); i++) {
    row[i]["active"] = (flowControlItems.at(i).enabled?1:0);
    row[i]["name"] = flowControlItems.at(i).name;
    row[i]["GpioPin"] = flowControlItems.at(i).port;
    row[i]["ImpPerLitre"] = (flowControlItems.at(i).ImpPerLitre);
    row[i]["l/min"]["data-id"] = String(flowControlItems.at(i).name.c_str()) + "_l/min";
    row[i]["total"]["data-id"] = String(flowControlItems.at(i).name.c_str()) + "_total";
  }

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";
}

void flowControl::loop() {
  if (millis() - lastCalculationTime > calculationPeriod) { // alle 5 Sekunden
    lastCalculationTime = millis();
    JsonDocument json;
    JsonArray rows = json.to<JsonArray>();

    for (uint8_t i=0; i< flowControlItems.size(); i++) {
      if (flowControlItems.at(i).ImpPerLitre > 0 and flowControlItems.at(i).enabled) {
        //calculate litre per minute, based on current counts of last calculationPeriod
        float litres = (float)flowControlItems.at(i).count / (float)flowControlItems.at(i).ImpPerLitre * 60.0 / (calculationPeriod / 1000);
        // add real flow litres since last calculation to litresCounter
        flowControlItems.at(i).litresCounter += (float)flowControlItems.at(i).count / (float)flowControlItems.at(i).ImpPerLitre;

        Config->logN(5, "FlowControl '%s': %u pulses/%u sec = %.3f Litres/min", flowControlItems.at(i).name.c_str(), flowControlItems.at(i).count, (calculationPeriod / 1000), litres);
        
        JsonObject obj = rows.add<JsonObject>();
        obj["name"] = flowControlItems.at(i).name;
        obj["l/min"] = litres;
        obj["total"] = flowControlItems.at(i).litresCounter;
        
        flowControlItems.at(i).count = 0; // Zähler zurücksetzen
      }
    }

    if (rows.size() > 0) {
      Config->logN(4, "Sending FlowControl data to MQTT: %s -> %s", mqtt->getTopic("flowcontrol", false).c_str(), json.as<String>().c_str());
      mqtt->Publish_String("flowcontrol", json.as<String>(), false);
      if (this->onValuesCallback) {
        this->onValuesCallback(json);
      }
    }
  }
}
