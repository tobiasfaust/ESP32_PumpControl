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
  flowercare_Callback: MyCallback
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
function MyCallback() {
  //global.CreateSelectionListFromInputField('input[type=number][id^=GpioPin]', [gpio]);
  //global.CreateSelectionListFromInputField('input[type=number][id*=ConfiguredPort]', [JSON.parse(configuredPorts)]);
  //global.handleRadioSelections();

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
