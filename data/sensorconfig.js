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
  data['cmd']['subaction'] = "sensorconfig";
  data['cmd']['callbackFn'] = "sensorconfig_Callback";
      
  global.requestData(data); 
}

// ************************************************
function MyCallback() {
  global.CreateSelectionListFromInputField('input[type=number][id^=GpioPin]', [gpio], JSON.parse(gpio_disabled));
  global.CreateSelectionListFromInputField('input[type=number][id^=AnalogPin]', [gpioanalog], JSON.parse(gpio_disabled));
  global.transformCheckboxes();
  global.handleRadioSelections();
  valveFn.validate_identifiers("moistureRows");

  document.querySelectorAll('#DataForm input:not([type=checkbox]):not([type=radio]), #DataForm select').forEach(element => {
    element.addEventListener('blur', global.showMustSaveDialog);
  });
    
  document.querySelectorAll('#DataForm input[type=checkbox], #DataForm input[type=radio]').forEach(element => {
  element.addEventListener('click', global.showMustSaveDialog);
  });
    
  global.initDataValues();

  document.querySelector("#loader").style.visibility = "hidden";
  document.querySelector("body").style.visibility = "visible";

  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "subscribe";
  data['cmd']['subaction'] = "ads1115_data";
  data["cmd"]["highlight"] = "true";
  
  global.requestData(data);
  
}

export function RefreshMeasurementMoisture() {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "sensor";
  data['cmd']['subaction'] = "requestMeasurementMoisture";
  global.requestData(data);
}

// ************************************************
// Registrierung der Callback-Funktionen
// Dieses Modul nutzt das Inversion-of-Control Callback-Registry aus Javascript.js
// Es werden folgende Callbacks aktiv beim Laden des Moduls registriert.
try {
  global.registerCallback('sensorconfig_Callback', MyCallback);
} catch(e) { console.error('Callback Registrierung fehlgeschlagen (sensorconfig):', e); }

// ************************************************
// Registrierung der Callback-Funktionen
// Dieses Modul nutzt das Inversion-of-Control Callback-Registry aus Javascript.js
// Es werden folgende Callbacks aktiv beim Laden des Moduls registriert.
try {
  global.registerCallback('sensorconfig_Callback', MyCallback);
} catch(e) { console.error('Callback Registrierung fehlgeschlagen (sensorconfig):', e); }

// ************************************************
