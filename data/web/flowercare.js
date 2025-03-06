import * as global from './Javascript.js';
import * as valveFn from './valvefunctions.js';

// ************************************************
export function init() {
  // Initiale Verbindung aufbauen
    global.connectWebSocket();
  
    // Warte bis die WebSocket-Verbindung aufgebaut ist
    let checkWebSocketInterval = setInterval(() => {
      if (global.ws && global.ws.readyState === WebSocket.OPEN) {
        clearInterval(checkWebSocketInterval);
        GetInitData();
      }
    }, 100);

}

// ************************************************
export const functionMap = {
  flowercare_Callback: MyCallback,
  onBleUpdate_Callback: onBleUpdate_cb
};

// ************************************************
function GetInitData() {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "GetInitData";
  data['cmd']['subaction'] = "flowercare";
  data['cmd']['callbackFn'] = "flowercare_Callback";
    
  global.requestData(data); 
}

// ************************************************
export function scanBLE() {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "flowercare";
  data['cmd']['subaction'] = "scan";
  data['cmd']['callbackFn'] = "flowercare_Callback";
    
  global.requestData(data); 
}

// ************************************************
function MyCallback(json) {
  //global.CreateSelectionListFromInputField('input[type=number][id^=GpioPin]', [gpio]);
  global.CreateSelectionListFromInputField('input[type=number][id*=ConfiguredPort]', [JSON.parse(configuredPorts)]);
  //global.handleRadioSelections();
  global.transformCheckboxes();
  valveFn.validate_identifiers("fc_relations_table");

  // anpassen der Update werte auf ein lesbares Format
  formatAllDates();  

  document.querySelectorAll('#DataForm input:not([type=checkbox]):not([type=radio]), #DataForm select').forEach(element => {
    element.addEventListener('blur', global.showMustSaveDialog);
  });
  
  document.querySelectorAll('#DataForm input[type=checkbox], #DataForm input[type=radio]').forEach(element => {
    element.addEventListener('click', global.showMustSaveDialog);
  });
  
  global.initDataValues();
  
  document.querySelector("#loader").style.visibility = "hidden";
  document.querySelector("body").style.visibility = "visible";
}

// ************************************************
// wird aufgerufen aus MyWebServer::flowerCareGetValuesCallback()
function onBleUpdate_cb(json) {
  formatAllDates();
}

// ************************************************
function formatAllDates() {
  const elements = document.querySelectorAll('[id$="lastLiveDataUpdate"], [id$="lastBatteryUpdate"]');
  elements.forEach(element => {
    formatDate(element);
  });
}

// ************************************************
export function ActivateDevice(id) {
  var obj = document.getElementById(id);
  
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "flowercare";
  data['cmd']['subaction'] = "activateDevice";
  data['cmd']['newState'] = (obj.checked?1:0);
  data['cmd']["item"] = obj.getAttribute('data-mac');
  
  global.requestData(data);
}

// ************************************************
export function ActivateRelation(id) {
  var obj = document.getElementById(id);
  
  // id = fc_relations_0.active
  var objMqttTopic = document.getElementById(id.replace("active", "mqtttopic"));
  var objPort = document.getElementById(id.replace("active", "ConfiguredPort"));

  if (objMqttTopic.value != "" && objPort.value != "") {
    var data = {};
    data['cmd'] = {};
    data['cmd']['action'] = "flowercare";
    data['cmd']['subaction'] = "activateRelation";
    data['cmd']['newState'] = (obj.checked?1:0);
    data['cmd']["item"] = objMqttTopic.value;
    data['cmd']["item2"] = objPort.value;
    
    global.requestData(data);
  } else {
    global.setResponse(false, "Please define a valid MQTT-Topic");
  }
}

// ************************************************
function formatDate(obj) {
  const intValue = parseInt(obj.innerText, 10);
  if (intValue > 0) {
    var diff = esp_uptime - intValue;
    var hours = Math.floor(diff / 3600000);
    var minutes = Math.floor((diff % 3600000) / 60000);
    var seconds = Math.floor((diff % 60000) / 1000);
    obj.innerText = `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
  }
}