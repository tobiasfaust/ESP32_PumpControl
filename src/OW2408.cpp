#include "OW2408.h"

ow2408::ow2408(){  }

void ow2408::init(uint8_t pin) {
  this->ow = new DS2408(pin); 
  this->findDevices();
  this->setup_devices();
  if (Config->GetDebugLevel() >=3) dbg.printf("OneWire DS2408 with %d devices initialized \n", this->device_count);
}

uint8_t ow2408::findDevices() {
  this->device_count = ow->find(&devices);
  return this->device_count;
}

void ow2408::setup_devices() {
    for(int index=0; index < this->device_count; index++) {
        ow->set_mode(this->devices[index], RESET_PIN_MODE(STROBE_MODE));
        ow->set_state(devices[index], 0x00); // all off
    }
}

String ow2408::print_device(uint8_t index) {
  char buffer[100] = {0};
  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "%02X %02X %02X %02X %02X %02X %02X %02X", devices[index][0], devices[index][1], devices[index][2], devices[index][3], devices[index][4], devices[index][5], devices[index][6], devices[index][7]);
  return buffer;
}

void ow2408::print_byte(uint8_t data) {
    for(int index=0; index<8; index++) {
        dbg.print(data & 1, BIN);
        data = data >> 1;
    }
}

bool ow2408::setOff(uint8_t port) {
  return handlePort(port, false);  
}

bool ow2408::setOn(uint8_t port) {
  return handlePort(port, true);
}

bool ow2408::setPort(uint8_t port, bool state) {
  return handlePort(port, state);
}

bool ow2408::handlePort(uint8_t port, bool state) {
  uint8_t DevPort = port % 8; // Device 0 = port 0-7; Device 1 = port 8-15
  uint8_t index = port / 8;
  bool ret;
  
  if ((index+1) > this->device_count) {
    if (Config->GetDebugLevel() >=2) { dbg.printf("requested OnWire index %d out of range\n", index); }
    return false; 
  }
  if (Config->GetDebugLevel() >=4) { dbg.printf("Schalte Device #%d  Port %d (%s)\n", index, DevPort, this->print_device(index).c_str()); }

  uint8_t currentstate = (this->ow->get_last_state(this->devices[index]));
  if (Config->GetDebugLevel() >=5)  { dbg.print(" STATE ALT="); print_byte(currentstate); }

  if(state) {
    // set ON
   currentstate |= (1 << DevPort);
  } else {
    // set OFF
    currentstate &= ~(1 << DevPort);
  }

  ret = this->ow->set_state(this->devices[index], currentstate);
  if (Config->GetDebugLevel() >=5) { dbg.print(" STATE NEU ="); print_byte(currentstate); dbg.println(""); }

  return ret;
}

bool ow2408::isValidPort(uint8_t port)  {
  // nur die Ports anzeigen die auch wirklich vorhanden sind
  uint8_t index = port / 8;

  if (index+1 > (this->device_count)) { return false;}
  return true; 
}


void ow2408::GetInitData(AsyncResponseStream *response) {
  String ret;
  JsonDocument json;
  
  json["data"].to<JsonObject>();
  JsonArray row = json["data"]["row"].to<JsonArray>();
  
  for(uint8_t i=0; i<this->device_count; i++) {
    row[i]["device"] = this->print_device(i);

    JsonArray ports = row[i]["ports"].to<JsonArray>();
    for(uint8_t j=0; j<8; j++) {
      ports[j]["port"] = j + " -> " + 140+(i*8)+j;
    }
  }

  json["response"].to<JsonObject>();
  json["response"]["status"] = 1;
  json["response"]["text"] = "successful";

  serializeJson(json, ret);
  response->print(ret);
}
