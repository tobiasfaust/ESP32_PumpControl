// ************************************************
window.addEventListener('DOMContentLoaded', init, false);
function init() {
  GetInitData();
}

// ************************************************
function GetInitData() {
  var data = {};
  data['action'] = "GetInitData";
  data['subaction'] = "status";
  requestData(JSON.stringify(data), false, MyCallback);
}

// ************************************************
function MyCallback() {
  document.querySelector("#loader").style.visibility = "hidden";
  document.querySelector("body").style.visibility = "visible";
}

// ************************************************
function RefreshI2C(id) {
  var data = {};
  data['action'] = "RefreshI2C";
  requestData(JSON.stringify(data), true);
}

function Refresh1Wire(id) {
  var data = {};
  data['action'] = "Refresh1Wire";
  requestData(JSON.stringify(data), true);
}