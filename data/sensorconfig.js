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
  sensorconfig_Callback: MyCallback
};

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

// ************************************************
