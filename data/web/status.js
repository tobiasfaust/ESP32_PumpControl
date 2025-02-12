import * as global from './Javascript.js';

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
  status_Callback: MyCallback,
  status_CallRebootPage: CallRebootPage
};

// ************************************************
function GetInitData() {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "GetInitData";
  data['cmd']['subaction'] = "status";
  data['cmd']['callbackFn'] = "status_Callback";
  
  global.requestData(data);  
}

// ************************************************
function MyCallback() {
  document.querySelector("#loader").style.visibility = "hidden";
  document.querySelector("body").style.visibility = "visible";
}

// ************************************************
export function RefreshI2C(id) {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "RefreshI2C";
  data['cmd']['highlight'] = "true";
  global.requestData(data);
}

export function Refresh1Wire(id) {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "Refresh1Wire";
  data['cmd']['highlight'] = "true";
  global.requestData(data);
}

// ************************************************
export function DoReboot() {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "reboot";
  data['cmd']['callbackFn'] = "status_CallRebootPage";
  global.requestData(data);
}

// ************************************************
export function DoReset() {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "reset";
  data['cmd']['callbackFn'] = "status_CallRebootPage";
  global.requestData(data);
}

// ************************************************
export function CallRebootPage(json) {
  window.location.href = "reboot.html";
}

// 