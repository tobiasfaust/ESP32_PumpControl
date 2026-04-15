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
  valveFn.validate_identifiers("fc_devices_table");

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

  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "subscribe";
  data['cmd']['subaction'] = "flowercare_data";
  data["cmd"]["highlight"] = "true";
  
  global.requestData(data);
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
export function forceUpdate(button) {
  var mac = button.closest('tr').querySelector('input[type="checkbox"][id$="active"]').getAttribute('data-mac');
  var active = button.closest('tr').querySelector('input[type="checkbox"][id$="active"]').checked;
  if (!active) {
    global.setResponse(false,"Device is not active. Please activate it first to be able to update it.");
    return;
  }

  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "flowercare";
  data['cmd']['subaction'] = "forceUpdate";
  data['cmd']['item'] = mac;

  global.requestData(data);
}


// ************************************************
function formatDate(obj, TimeDiffInt) {
  let intValue = 0;
  if (TimeDiffInt) { intValue = TimeDiffInt; } else { intValue = parseInt(obj.innerText, 10); }
  if (intValue > 0) {
    var diff = esp_uptime - intValue;
    var hours = Math.floor(diff / 3600000);
    var minutes = Math.floor((diff % 3600000) / 60000);
    var seconds = Math.floor((diff % 60000) / 1000);
    obj.innerText = `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
    // sichere originalwert im Attribute data-timestamp
    obj.setAttribute('data-timestamp', intValue);
  }
}

// ************************************************
//rufe folgende funktion jede sekunde auf, um die Zeiten zu aktualisieren
setInterval(updateTimestamps, 1000);
function updateTimestamps() {
  document.querySelectorAll('[id$="lastLiveDataUpdate"], [id$="lastBatteryUpdate"]').forEach(element => {
    let TimeDiffInt = parseInt(element.getAttribute('data-timestamp'), 10);

    if (TimeDiffInt) {
      TimeDiffInt -= 1000; // ändere den Timestamp um 1 Sekunde
      formatDate(element, TimeDiffInt); // aktualisiere die Anzeige
    }
  });
}

// ************************************************
// Registrierung der Callback-Funktionen
// Dieses Modul nutzt das Inversion-of-Control Callback-Registry aus Javascript.js
// Es werden folgende Callbacks aktiv beim Laden des Moduls registriert.
try {
  global.registerCallback('flowercare_Callback', MyCallback);
  global.registerCallback('onBleUpdate_Callback', onBleUpdate_cb);
} catch(e) { console.error('Callback Registrierung fehlgeschlagen (flowercare):', e); }