import * as global from './Javascript.js';

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

// Hinweis: functionMap entfernt, stattdessen Registrierung via registerCallback am Ende

// ************************************************
function GetInitData() {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "GetInitData";
  data['cmd']['subaction'] = "baseconfig";
  data['cmd']['callbackFn'] = "basisconfig_Callback";
    
  global.requestData(data); 
}

export function RefreshWifiAPs(id) {
  global.setResponse(true, "Refreshing WiFi APs, please wait...");
  
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "RefreshWifiAPs";
  global.requestData(data);
}

// ************************************************
function MyCallback() {
  global.CreateSelectionListFromInputField('input[type=number][id^=GpioPin]', [gpio], JSON.parse(gpio_disabled));
  global.CreateSelectionListFromInputField('input[type=number][id*=ConfiguredPort]', [JSON.parse(configuredPorts)]);
  global.handleRadioSelections();

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
// Registrierung der Callback-Funktionen
// Dieses Modul nutzt das Inversion-of-Control Callback-Registry aus Javascript.js
// Es werden folgende Callbacks aktiv beim Laden des Moduls registriert.
try {
  global.registerCallback('basisconfig_Callback', MyCallback);
} catch(e) { console.error('Callback Registrierung fehlgeschlagen (baseconfig):', e); }
