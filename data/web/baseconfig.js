var ReleaseInfo;

// ************************************************
window.addEventListener('DOMContentLoaded', init, false);
function init() {
  GetInitData();
}

// ************************************************
function GetInitData() {
  var data = {};
  data.action = "GetInitData";
  data.subaction = "baseconfig";
  requestData(JSON.stringify(data), false, MyCallback);
}

// ************************************************
function MyCallback() {
  CreateSelectionListFromInputField('input[type=number][id^=GpioPin]', [gpio]);
  CreateSelectionListFromInputField('input[type=number][id*=ConfiguredPort]', [configuredPorts]);
  SetUpdateURL()
  FetchReleaseInfo();
  document.querySelector("#loader").style.visibility = "hidden";
  document.querySelector("body").style.visibility = "visible";
}

// ************************************************

function SetUpdateURL() {
  var u = document.getElementById("au_url");
  u.value = update_url;
}

function FetchReleaseInfo() {
  fetch( update_url, {})
      .then (response => response.json())
      .then (json =>  {
        ProcessReleaseInfo(json)
      })
}

function ProcessReleaseInfo(json) {
	var _parent, _select, _optgroup_dev, _optgroup_pre, _optgroup_prd, _option;
  this.ReleaseInfo = json;
  
  _select = document.getElementById('releases');
  _select.replaceChildren();
  
  _optgroup_dev = document.createElement('optgroup');
  _optgroup_pre = document.createElement('optgroup');
  _optgroup_prd = document.createElement('optgroup');
 
  _optgroup_dev.label = "Development";
  _optgroup_pre.label = "Prelive";
  _optgroup_prd.label = "Produktiv";
  
  if (Array.isArray(json)) {
    for (var i=0; i < json.length; i++) {
    	//console.log(json[i])
      _option = document.createElement('option');
      _option.value = json[i].subversion; //['download-url'];
      _option.text = json[i].name + " (" + json[i].subversion + ")";
      //_option.selected
      if (json[i].stage == 'DEV')  { _optgroup_dev.appendChild(_option); }
      if (json[i].stage == 'PRE')  { _optgroup_pre.appendChild(_option); }
      if (json[i].stage == 'PROD') { _optgroup_prd.appendChild(_option); }
    } 
  }
  
  _select.add( _optgroup_prd );
  _select.add( _optgroup_pre );
  _select.add( _optgroup_dev ); 
}

async function FetchRelease() {
  var r = document.getElementById('releases')
  var releaseJson = GetSpecificReleaseJson(r.value);
  
  // show loader
  document.querySelector("#loader").style.visibility = "visible"; 

  // fetch new release firmware
  const ReleaseBlob = await fetch( releaseJson['download-url'], {
  	responseType: 'blob'
  		})
      .then (response =>  response.blob())
      ;
  
  if(ReleaseBlob) {
    setResponse(true, "new Firmware successfully downloaded");
  
    //upload releaseinformation (to show in Web-Header)
    var JsonText = JSON.stringify(releaseJson);
    var JsonBlob = new Blob([JsonText], {type:"text/plain"});
    await UploadFile(JsonBlob, "ESPUpdate.json", ""); 
    
    // Upload new release firmware
    setResponse(true, "please wait to upload new firmware");
    await InstallRelease(ReleaseBlob);

    setResponse(true, "please wait a few seconds to reload");

    setTimeout(() => {
      top.location.href="/";
    }, 5000);

  } else {
    setResponse(false, "new Firmware download failed");
  }
} 
 
async function InstallRelease(BinaryBlob) {
  const formData = new FormData();
  formData.append("firmware", BinaryBlob, "firmware.bin");
    
  await fetch('/update', {
      method: 'POST',
      body: formData,
    })
}
 
function GetSpecificReleaseJson(subversion) {
  if (Array.isArray(this.ReleaseInfo)) {
    for (var i=0; i < this.ReleaseInfo.length; i++) {
      if (subversion == this.ReleaseInfo[i].subversion) return this.ReleaseInfo[i];
    }
  }
}