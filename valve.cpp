#include "valve.h"

valve::valve() : enabled(true), active(false), ValveType(NONE), autooff(0), reverse(false) {
}

void valve::init(valveHardware* vHW, uint8_t Port, String SubTopic) {
  this->valveHWClass = vHW;
  bool ret = valveHWClass->RegisterPort(this->myHWdev, Port);
  if (!ret) { Serial.printf("Cannot locate port %d, set port as disabled \n", Port); this->enabled = false; }
  this->ValveType = NORMAL;
  this->port1 = Port;
  this->subtopic = SubTopic;
}

void valve::AddPort1(valveHardware* Device, uint8_t Port1) {
  this->valveHWClass = Device;
  bool ret = Device->RegisterPort(this->myHWdev, Port1);
  if (!ret) { Serial.printf("Cannot locate port %d, set port as disabled \n", Port1); this->enabled = false; }
  this->port1 = Port1;  
}

void valve::AddPort2(valveHardware* Device, uint8_t Port2) {
  bool ret = Device->RegisterPort(this->myHWdev, Port2);
  if (!ret) { Serial.printf("Cannot locate port %d, set port as disabled \n", Port2); this->enabled = false; }
  this->port2 = Port2;
}

void valve::SetActive(bool value) {
  this->enabled = value;
}

void valve::SetReverse(bool value) {
  this->reverse = value;
}

void valve::SetAutoOff(uint16_t value) {
  this->autooff = value;
}

bool valve::OnForTimer(int duration) {
  bool ret= false;
  if (enabled && ActiveTimeLeft() < duration) {ret = this->HandleSwitch(true, duration);}
  if (duration == 0) { ret = this->SetOff(); }
  return ret;
}

bool valve::SetOn() {
  bool ret = false;
  if (this->enabled && !this->active) {
    if (this->autooff > 0) {
      ret = this->HandleSwitch(true, this->autooff);
    } else {
      ret = this->HandleSwitch(true, 0);
    }
  }
  return ret;
}

bool valve::SetOff() {
  bool ret = false;
  if (this->active) {ret = this->HandleSwitch(false, 0);}
  return ret;
}

bool valve::HandleSwitch (bool state, int duration) {
  char buffer[50] = {0};
  memset(buffer, 0, sizeof(buffer));
  if (ValveType == NORMAL) {
    Serial.printf("Schalte Standard Ventil %s: Port %d (0x%02X) \n", (state?"An":"Aus"), this->port1, myHWdev->i2cAddress);
    valveHWClass->SetPort(myHWdev, this->port1, state, this->reverse);
  } else if (ValveType == BISTABIL) {
    Serial.printf("Schalte Bistabiles Ventil %s: Port %d/%d, ms: %d/%d (0x%02X) \n", (state?"An":"Aus"), port1, port2, port1ms, port2ms, myHWdev->i2cAddress);
    valveHWClass->SetPort(myHWdev, this->port1, this->port2, state, this->reverse, (state?this->port1ms:this->port2ms));
  }

  this->active = state;
  
  if (state && duration && duration>0) {
    startmillis = millis();
    lengthmillis = duration * 1000;
  } else {
    startmillis = lengthmillis = 0;
  }

  if(mqtt) {
    snprintf (buffer, sizeof(buffer), "%s/state", this->subtopic.c_str());
    mqtt->Publish_Bool(buffer, state);
  }

  return true;
}

int valve::ActiveTimeLeft() {
  return _max((lengthmillis - (millis() - startmillis)),0);
}

void valve::SetValveType(String type) {
  if (type == "n") { ValveType = NORMAL; }
  else if (type=="b") { ValveType = BISTABIL; }
  else { ValveType = NONE; }
}

String valve::GetValveType() {
  if (ValveType == NORMAL) { return "n"; }
  else if (ValveType == BISTABIL) { return "b"; }
  else { return ""; }
}

uint8_t valve::GetPort1() {
  return port1;
}

uint8_t valve::GetPort2() {
  return port2;
}

void valve::loop() {
  if (ActiveTimeLeft()==0) {
    //Serial.print("on-for-timer abgelaufen: Pin");Serial.println(i);
    SetOff();
  }
}
