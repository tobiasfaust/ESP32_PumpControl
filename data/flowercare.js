import * as global from './Javascript.js';
import * as valveFn from './valvefunctions.js';

// ************************************************
export function init() {
  global.connectWebSocket();
  let checkWebSocketInterval = setInterval(() => {
    if (global.ws && global.ws.readyState === WebSocket.OPEN) {
      clearInterval(checkWebSocketInterval);
      GetInitData();
    }
  }, 100);
}

// functionMap entfernt -> Registrierung am Ende

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
  //global.CreateSelectionListFromInputField('input[type=number][id^=GpioPin]', [gpio], JSON.parse(gpio_disabled));
  //global.CreateSelectionListFromInputField('input[type=number][id*=ConfiguredPort]', [JSON.parse(configuredPorts)]);
  //global.handleRadioSelections();
  global.transformCheckboxes();
  //valveFn.validate_identifiers("fc_relations_table");

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

// ************************************************
// Registrierung der Callback-Funktionen
// Dieses Modul nutzt das Inversion-of-Control Callback-Registry aus Javascript.js
// Es werden folgende Callbacks aktiv beim Laden des Moduls registriert.
try {
  global.registerCallback('flowercare_Callback', MyCallback);
  global.registerCallback('onBleUpdate_Callback', onBleUpdate_cb);
} catch(e) { console.error('Callback Registrierung fehlgeschlagen (flowercare):', e); }