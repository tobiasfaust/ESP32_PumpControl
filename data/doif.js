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
  data['cmd']['subaction'] = "doif";
  data['cmd']['callbackFn'] = "doif_Callback";

  global.requestData(data);
}

// ************************************************
function MyCallback() {
  //global.CreateSelectionListFromInputField('input[type=number][id^=GpioPin]', [gpio], JSON.parse(gpio_disabled));
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
export function ActivateRelation(obj) {
  var id = obj.id;

  // id = fc_relations_0.active
  var objMqttTopic = document.getElementById(id.replace("active", "mqtttopic"));
  var objPort = document.getElementById(id.replace("active", "ConfiguredPort"));

  if (objMqttTopic.value != "" && objPort.value != "") {
    var data = {};
    data['cmd'] = {};
    data['cmd']['action'] = "doif";
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
// check if the entered pattern is a valid RegExp
export function checkRegExp(obj) {
  try {
    new RegExp(obj.value);
  } catch (e) {
    global.setResponse(false, "Invalid RegExp pattern");
    obj.focus();
  }
}

/*************************************************
 * Registrierung der Callback-Funktion
 *************************************************/
try {
  global.registerCallback('doif_Callback', MyCallback);
} catch(e) { console.error('Callback Registrierung fehlgeschlagen (doif):', e); }
