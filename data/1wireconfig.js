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
  data['cmd']['subaction'] = "1wireconfig";
  data['cmd']['highlight'] = "true";
  data['cmd']['callbackFn'] = "onewireconfig_Callback";
    
  global.requestData(data); 
}

// ************************************************

function MyCallback() {
  valveFn.validate_identifiers("maintable");

  document.querySelector("#loader").style.visibility = "hidden";
  document.querySelector("body").style.visibility = "visible";
}

// ************************************************
// Registrierung der Callback-Funktionen
// Dieses Modul nutzt das Inversion-of-Control Callback-Registry aus Javascript.js
// Es werden folgende Callbacks aktiv beim Laden des Moduls registriert.
try {
  global.registerCallback('onewireconfig_Callback', MyCallback);
} catch(e) { console.error('Callback Registrierung fehlgeschlagen (1wireconfig):', e); }
// ************************************************
