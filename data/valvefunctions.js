import * as global from './Javascript.js';

/*******************************
copy first row of table and add it as clone 
*******************************/
export function addrow(tableID) { 
  var _table = document.getElementById(tableID);
  var firstrow;
  for( var i=0; i< _table.rows.length; i++) { 
    if (GetParentObject(_table.rows[i], "THEAD")) continue;
    firstrow = i;
  }
  
  _table.rows[firstrow].style.display = '';
  var new_row = _table.rows[firstrow].cloneNode(true);
  _table.appendChild(new_row);
  validate_identifiers(tableID);
}


/*******************************
delete a row in table
*******************************/
export function delrow(object) { 
  var table = GetParentObject(object, 'TABLE');
  var rowIndex = GetParentObject(object, 'TR').rowIndex;
  var rowFirst=0;
  for( var i=0; i< table.rows.length; i++) { 
    if (GetParentObject(table.rows[i], "THEAD")) continue;
    rowFirst = i; break;
  }
  if (table.rows.length > rowFirst+1) {
    // erste Zeile ist das Template + Header, darf nicht entfernt werden
    table.deleteRow(rowIndex)
    validate_identifiers(table.id);
  }
}


/*******************************
recalculate all id´s, name´s
*******************************/
export function validate_identifiers(tableID) {
  var table = document.getElementById(tableID); 
  var counter=1;
  for( var i=0; i< table.rows.length; i++) { 
    var row = table.rows[i];
    if (GetParentObject(row, "THEAD")) continue;
    
    row.cells[0].innerHTML = counter;
    var objects = row.querySelectorAll('label, input, select, div, td');
    for( var j=0; j< objects.length; j++) {
      if (objects[j].name) {objects[j].name = objects[j].name.replace(/(\d+)/, counter-1);}
      if (objects[j].id) {objects[j].id = objects[j].id.replace(/(\d+)/, counter-1);}
      if (objects[j].htmlFor) {objects[j].htmlFor = objects[j].htmlFor.replace(/(\d+)/, counter-1);}
    }
    counter++;
  }
}

/*******************************
return the first parent object of tagName, e.g. TR
*******************************/
function GetParentObject(object, TargetTagName) {
  if (object.tagName == TargetTagName) {return object;}
  else if (object.parentNode === null) { return false;}
  else { return GetParentObject(object.parentNode, TargetTagName); }
}

/*******************************
return the port-value of selected row, 
object: anyone object of that row 
*******************************/
function GetPortOfRow(object) {
  var port = 0;
  var row = GetParentObject(object, 'TR')
  var objects = document.querySelectorAll('select[id*=AllePorts][name=port_a]');

  for( var i=0; i< objects.length; i++) {
    if(global.isVisible(objects[i]) && row == GetParentObject(objects[i], 'TR')) {
      port = objects[i].value;
    }
  }
  
  return port;
}

/************************************************
the "active" checkbox has pressed
*************************************************/
export function ChangeEnabled(object) {
  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "EnableValve";
  data['cmd']['newState'] = object.checked;
  data['cmd']['item'] = GetPortOfRow(object);
        
  global.requestData(data); 
}

/************************************************
set valve on or off
*************************************************/
export function ChangeValve(object) {
  var btn = document.getElementById(object.id);

  var data = {};
  data['cmd'] = {};
  data['cmd']['action'] = "SetValve";
  data['cmd']['subaction'] = object.id;
  data['cmd']['newState'] =  (btn.value.toLowerCase().includes("on") ? 1 : 0);
  data['cmd']['item'] = GetPortOfRow(object);
        
  global.requestData(data); 
}

/************************************************
the "active" checkbox was press
*************************************************/
export function ChangeType(object) {
  var _obj_n, _obj_b; 
  var row = GetParentObject(object, 'TR')
  var val = object.value;

  var objects = document.querySelectorAll('div[id*=typ_]');

  for( var i=0; i< objects.length; i++) {
    if(row == GetParentObject(objects[i], 'TR')) {
      if (objects[i].id.match(/typ_n/)) {_obj_n = objects[i];}
      if (objects[i].id.match(/typ_b/)) {_obj_b = objects[i];}
    }
  }
 
  if (val == 'b') {
    // Typ "Bistabil"
    _obj_n.classList = "hide";
    _obj_b.classList = "";
  } else if (val == 'n'){
    // Typ "normal"
    _obj_n.classList = "";
    _obj_b.classList = "hide";
  } 
}
