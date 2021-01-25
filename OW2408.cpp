#include "OW2408.h"

ow2408::ow2408(): debugmode(0) {  }

void ow2408::init(uint8_t pin) {
  ow = new DS2408(pin); 
  this->findDevices();
  this->setup_devices();
  if (debugmode >=3) Serial.printf("OneWire DS2408 with %d devices initialized \n", this->device_count);
}

void ow2408::findDevices() {
  this->device_count = ow->find(&devices);
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
        Serial.print(data & 1, BIN);
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
    if (debugmode >=2) { Serial.printf("requested OnWire index %d out of range\n", index); }
    return false; 
  }
  if (debugmode >=4) { Serial.printf("Schalte Device #%d  Port %d (%s)\n", index, DevPort, this->print_device(index).c_str()); }

  uint8_t currentstate = (ow->get_state(this->devices[index]));
  if (debugmode >=5) { Serial.print(" STATE="); print_byte(currentstate); }

  if(state) {
    // set ON
   currentstate |= (1 << DevPort);
  } else {
    // set OFF
    currentstate &= ~(1 << DevPort);
  }

  ret = ow->set_state(this->devices[index], currentstate);
  if (debugmode >=5) { Serial.print(" STATE NEU ="); print_byte(currentstate); Serial.println(""); }

  return ret;
  
}

void ow2408::setDebugMode(uint8_t debugmode) {
  this->debugmode = debugmode;
}

bool ow2408::isValidPort(uint8_t port)  {
  // nur die Ports anzeigen die auch wirklich vorhanden sind
  uint8_t index = port / 8;

  if (index+1 > (this->device_count)) { return false;}
  return true; 
}

void ow2408::GetWebContent1Wire(WM_WebServer* server) {
  char buffer[200] = {0};
  memset(buffer, 0, sizeof(buffer));
  String html = "";

  html.concat("<table id='maintable' class='editorDemoTable'>\n");
  html.concat("<thead>\n");
  html.concat("<tr>\n");
  html.concat("<td style='width: 25px;'>Nr</td>\n");
  html.concat("<td style='width: 75px;'>Typ</td>\n");
  html.concat("<td style='width:150px;'>ID</td>\n");
  html.concat("<td style='width:  80px;'>Port</td>\n");
  html.concat("</tr>\n");
  html.concat("</thead>\n");
  server->sendContent(html.c_str()); html = "";

  for(uint8_t i=0; i<this->device_count; i++) {
    html.concat("<tr>\n");
    sprintf(buffer, "  <td>%d</td>\n", i+1);
    html.concat(buffer);
    html.concat("  <td>DS2408</td>\n");
    sprintf(buffer, "  <td>%s</td>\n", this->print_device(i).c_str());
    html.concat(buffer);

    html.concat("  <td><table>\n");
    for(uint8_t j=0; j<8; j++) {
      sprintf(buffer, "    <tr><td>Port %d -> %d</td></tr>\n", j, 140+(i*8)+j);
      html.concat(buffer);
    }
    html.concat("</table></td>\n");

    html.concat("</tr>\n");
    server->sendContent(html.c_str()); html = "";
  }

  html.concat("</table>\n");
  server->sendContent(html.c_str()); html = "";
}
