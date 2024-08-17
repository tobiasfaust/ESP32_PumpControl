// https://jsfiddle.net/tobiasfaust/uc1jfpgb/

var DirJson;

window.addEventListener('load', initHandleFS, false);
function initHandleFS() {
  document.querySelector("#loader").style.visibility = "hidden";
  document.querySelector("body").style.visibility = "visible";
  init("/");
}

function init(startpath) {
  requestListDir(startpath);
  obj = document.getElementById('fullpath').innerHTML = ''; // div 
  obj = document.getElementById('filename').value = ''; // input field
  obj = document.getElementById('content').value = '';
  
}

// ***********************************
// Ajax Request to update  
// ***********************************
function requestListDir(startpath) {
  var data = {};
  data['action'] = "handlefiles";
  data['subaction'] = "listDir"
  //ajax_send(JSON.stringify(data));
  
  var http = null;
  if (window.XMLHttpRequest)  { http =new XMLHttpRequest(); }
  else                        { http =new ActiveXObject("Microsoft.XMLHTTP"); }
  
  if(!http){ alert("AJAX is not supported."); return; }
 
  var url = '/ajax';
  var params = 'json=' + JSON.stringify(data);
  
  http.open('POST', url, true);
  http.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
  http.onreadystatechange = function() { //Call a function when the state changes.
    if(http.readyState == 4 && http.status == 200) {
      DirJson = JSON.parse(http.responseText);
      listFiles(startpath);
    } 
  }
  http.send(params);
}

// ***********************************
// show content of fetched file 
// ***********************************
function setContent(string, file) {
  obj = document.getElementById('fullpath').innerHTML = file; // div 
  obj = document.getElementById('filename').value = basename(file); // input field
  
  if (file.endsWith("json")) {
    obj = document.getElementById('content').value = JSON.stringify(JSON.parse(string), null, 2);
  } else {
    obj = document.getElementById('content').value = string;
  }
}

// ***********************************
// fetch file from host
// ***********************************
function fetchFile(file) {
  obj = document.getElementById('content').value = "loading "+file+"...";
  
  fetch(file)
  .then(response => response.text())
  .then(textString => setContent(textString, file));
}

// ***********************************
// show directory structure 
// ***********************************
function listFiles(path) {
  var table = document.querySelector('#files'),
      row = document.querySelector('#NewRow'),
      tr_tpl, DirJsonLocal;
  
  // cleanup table
  table.replaceChildren();
  
  // get the right part
  for(let i = 0; i < DirJson.length; i++) { 
    if (DirJson[i].path == path) {
      DirJsonLocal = DirJson[i]
    }
  }
  
  // show path information
  document.getElementById('path').innerHTML = path;
  
  // set "back" item if not root
  if (path != '/') {
    tr_tpl = document.importNode(row.content, true);
    cells = tr_tpl.querySelectorAll("td");
      cells.forEach(function (item, index) {
        var text = item.innerHTML;
        var oc = "listFiles('" + getParentPath(path) + "')"
        text = text.replaceAll("{file}", '..');
        item.innerHTML = text;
        item.setAttribute('onClick', oc);
    });
    table.appendChild(tr_tpl);
  }
  
  // show files
  DirJsonLocal.content.forEach(function (file) {
    // template "laden" (lies: klonen)
    tr_tpl = document.importNode(row.content, true);
    cells = tr_tpl.querySelectorAll("td");
    cells.forEach(function (item, index) {
      var text = item.innerHTML;
      var oc;
      
      if(file.isDir == 0) {
        oc = item.getAttribute('onClick');
        var newPath = DirJsonLocal.path + "/" + file.name;
        if (newPath.startsWith("//")) {newPath = newPath.substring(1)}
        oc = oc.replaceAll("{fullpath}", newPath);
        text = text.replaceAll("{file}", file.name);
      } else if(file.isDir == 1) {
        var newPath = DirJsonLocal.path + "/" + file.name;
        if (newPath.startsWith("//")) {newPath = newPath.substring(1)}
        oc = "listFiles('" + newPath + "')"
        text = text.replaceAll("{file}", file.name + "/");
      }
      
      
      item.innerHTML = text;
      item.setAttribute('onClick', oc);
    });
    table.appendChild(tr_tpl);
  })
 }

// ***********************************
// returns parent path: '/regs/web' -> '/regs' 
// ***********************************
function getParentPath(path) {
  var ParentPath, PathArray;
  
  PathArray = path.split('/')
  PathArray.pop()
  if (PathArray.length == 1) { ParentPath = '/' }
  else { ParentPath = PathArray.join('/')}
  return ParentPath
}

// ***********************************
// extract the filename from path
// ***********************************
function basename(str) {
    return str.split('\\').pop().split('/').pop();
}

// ***********************************
// return true if valid json, otherwise false
// ***********************************
function validateJson(json) {
  try {
    JSON.parse(json);
    return true;
  } catch {
      return false;
  }
}

// ***********************************
// download content of textarea as filename on local pc
// ***********************************
function downloadFile() {
  var textToSave = document.getElementById("content").value;
  var textToSaveAsBlob = new Blob([textToSave], {type:"text/plain"});
  var textToSaveAsURL = window.URL.createObjectURL(textToSaveAsBlob);
  var fileNameToSaveAs = document.getElementById("filename").value;
 
  if (fileNameToSaveAs != '') { 
    var downloadLink = document.createElement("a");
    downloadLink.download = fileNameToSaveAs;
    downloadLink.innerHTML = "Download File";
    downloadLink.href = textToSaveAsURL;

     downloadLink.onclick = destroyClickedElement;
    downloadLink.style.display = "none";
    document.body.appendChild(downloadLink);
    downloadLink.click();
  } else { setResponse(false, 'Filename is empty, Please define it.');}
}

function destroyClickedElement(event)
{
    document.body.removeChild(event.target);
}

// ***********************************
// store content of textarea
// ***********************************
function uploadAsFile() {
  var textToSave = document.getElementById("content").value;
  var textToSaveAsBlob = new Blob([textToSave], {type:"text/plain"});
  var fileNameToSaveAs = document.getElementById("filename").value;
  var pathOfFile = document.getElementById('path').innerHTML;
  
  if (fileNameToSaveAs != '') {
    if (fileNameToSaveAs.toLowerCase().endsWith('.json')) {
      if (!validateJson(textToSave)) {
        setResponse(false, 'Json invalid')
        return;
      }
    }

    setResponse(true, 'Please wait for saving ...');
    
    UploadFile(textToSaveAsBlob, fileNameToSaveAs, pathOfFile);
  
  } else { setResponse(false, 'Filename is empty, Please define it.');}
}

async function deleteAFile(file) {
  if (file != '') {
    var data = {};
    data['action'] = 'handlefiles';
    data['subaction'] = "deleteFile";
    data['filename'] = file;

    setResponse(true, 'Please wait for deleting ...');
    requestData(JSON.stringify(data));
  } else { setResponse(false, 'Filename is empty, Please define it.');}
}

async function deleteFile() {
  var pathOfFile = document.getElementById('path').innerHTML;
  var fileName = document.getElementById("filename").value;
  
  await deleteAFile(pathOfFile + '/' + fileName);
  init(pathOfFile);
}

// ***********************************
// backup complete filesystem of ESP by zipfile
//
// https://gist.github.com/noelvo/4502eea719f83270c8e9
// ***********************************
function backup() {
  var url = [];
  
  for(let i = 0; i < DirJson.length; i++) { 
    DirJson[i].content.forEach(function (file) {
		  if (file.isDir==0) {
	      //console.log(DirJson[i].path, file.name)
				url.push(DirJson[i].path + "/" + file.name)
      }
    })
  }
  compressed_img(url, "backup");
}

function compressed_img(urls, nombre) {
  var zip = new JSZip();
  var count = 0;
  var name = nombre + ".zip";
  urls.forEach(function(url){
    JSZipUtils.getBinaryContent(url, function (err, data) {
      if(err) {
         throw err; 
      }
       zip.file(url, data,  {binary:true});
       count++;
       if (count == urls.length) {
         zip.generateAsync({type:'blob'}).then(function(content) {
            saveAs(content, name);
         });
       }
      });
  });
}