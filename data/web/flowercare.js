import * as global from './Javascript.js';
import * as valveFn from './valvefunctions.js';

// ************************************************

export function init1() {
  var json = {
    "cmd": {
        "action": "GetInitData",
        "subaction": "flowercare",
        "callbackFn": "flowercare_Callback"
    },
    "data": {
        "flowercare": [
            {
                "address": "c4:7c:8d:62:8e:04",
                "mqtttopic": "flowercare/c4:7c:8d:62:8e:04",
                "battery": 49,
                "firmwareVersion": "2.7.0",
                "temperature": 17.8,
                "moisture": 0,
                "brightness": 92,
                "fertility": 0,
                "lastLiveDataUpdate": 347962,
                "lastBatteryUpdate": 346362,
                "failedReads": 0,
                "active": {
                    "checked": true,
                    "data-mac": "c4:7c:8d:62:8e:04"
                }
            },
            {
                "address": "c4:7c:8d:62:8e:a2",
                "mqtttopic": "flowercare/c4:7c:8d:62:8e:a2",
                "battery": 50,
                "firmwareVersion": "2.7.0",
                "temperature": 17.4,
                "moisture": 0,
                "brightness": 113,
                "fertility": 0,
                "lastLiveDataUpdate": 353062,
                "lastBatteryUpdate": 351512,
                "failedReads": 0,
                "active": {
                    "checked": true,
                    "data-mac": "c4:7c:8d:62:8e:a2"
                }
            },
            {
                "address": "c4:7c:8d:62:8e:76",
                "mqtttopic": "flowercare/c4:7c:8d:62:8e:76",
                "battery": 60,
                "firmwareVersion": "2.7.0",
                "temperature": 17,
                "moisture": 1,
                "brightness": 110,
                "fertility": 0,
                "lastLiveDataUpdate": 356291,
                "lastBatteryUpdate": 51559,
                "failedReads": 1,
                "active": {
                    "checked": true,
                    "data-mac": "c4:7c:8d:62:8e:76"
                }
            }
        ],
        "fc_relations": [
            {
                "active": {
                    "checked": false
                },
                "threshold": 30,
                "duration": 60
            },
            {
                "active": {
                    "checked": false
                },
                "threshold": 30,
                "duration": 60
            }
        ]
    },
    "js": {
        "esp_uptime": 361485,
        "gpio_disabled": "[221,222,0]",
        "availablePorts": "[]",
        "configuredPorts": "[{\"port\":216, \"name\":\"Valve1\"},{\"port\":217, \"name\":\"Valve2\"}]"
    },
    "response": {
        "status": 1,
        "text": "successful"
    }
}

  MyCallback(json);
}

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