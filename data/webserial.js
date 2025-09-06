let ws;    // websocket handle
let reconnectInterval = 5000; // 5 seconds interval to reconnect websocket connection
let logLineCount = 0;
let logRawLines = [];

/*****************************************************************
 * Call this init function at first after complete loading html content
 * Initialize the WebSocket connection and subscribe to log data
 *****************************************************************/
export function init() {
  // Initiale Verbindung aufbauen
  connectWebSocket();

  // Warte bis die WebSocket-Verbindung aufgebaut ist
  let checkWebSocketInterval = setInterval(() => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      clearInterval(checkWebSocketInterval);
      GetInitData();
    }
  }, 100);
}

/*****************************************************************
 * Subscribe to log data from server and set callback function
 * ***************************************************************/
function GetInitData() {
    // Log subscribe request 
    var data = {};
    data['cmd'] = {};
    data['cmd']['action'] = "subscribe";
    data['cmd']['subaction'] = "log_data";
    data['cmd']['callbackFn'] = "MyCallback";
    requestData(data);
}

/*****************************************************************
 * Connect to WebSocket server
 * ***************************************************************/
export function connectWebSocket() {
  // if window is about to be closed, close the websocket connection
  window.addEventListener('beforeunload', function() {
    if (ws) {
      ws.close();
    }
  }, false);

  addLogLine('Open WebSocket Connection at: ' + getWsUrlFromQuery() || '/ws');
  ws = new WebSocket(getWsUrlFromQuery() || '/ws');
  
  ws.onopen = function() {
    addLogLine('WebSocket connection opened');
    updateWsStatus();
  };

  ws.onmessage = function(event) {
    const json = JSON.parse(event.data);
    //console.log('Received JSON:', json);
    // Use a function map or window to resolve callback functions
    if (json.cmd.callbackFn && typeof eval(json.cmd.callbackFn) === 'function') {
      eval(json.cmd.callbackFn)(json);
    } else {
      console.warn('No valid callback function found for:', json.cmd.callbackFn);
    }
  };

  ws.onclose = function() {
    addLogLine('WebSocket connection closed, attempting to reconnect in ' + reconnectInterval / 1000 + ' seconds');
    ws.close();
    updateWsStatus();
    init()
  };

  ws.onerror = function(error) {
    addLogLine('WebSocket error:', error);
    ws.close();
    updateWsStatus();
    init();
  };
}

/***********************************************************************
 * Get WebSocket URL from query parameter (if provided)
 * Example: ?ws_url=ws://example.com/socket
 ***********************************************************************/
function getWsUrlFromQuery() {
  const params = new URLSearchParams(window.location.search);
  return params.get('ws_url');
}

/*****************************************************************************************
 * central function to send data to server
 * @param {*} json -> json object to send
 * @returns {*} void
******************************************************************************************/
function requestData(json) {
  if (typeof ws !== 'undefined' && ws.readyState === WebSocket.OPEN) {
    console.log('WebSocket is open, sending data:', json);
    ws.send(JSON.stringify(json));
  } else {
    console.log('WebSocket not open, could not send data');
  }
}

/*****************************************************************
 * Theme switching with icon and button contrast
 *****************************************************************/
export function toggleTheme() {
  const body = document.body;
  const btnTheme = document.getElementById('btn-theme');
  if (body.classList.contains('bright')) {
    body.classList.remove('bright');
    btnTheme.innerHTML = '<i class="fa fa-sun-o"></i>';
    btnTheme.classList.remove('theme-bright');
    btnTheme.classList.add('theme-dark');
  } else {
    body.classList.add('bright');
    btnTheme.innerHTML = '<i class="fa fa-moon-o"></i>';
    btnTheme.classList.remove('theme-dark');
    btnTheme.classList.add('theme-bright');
  }
}

/*****************************************************************
 * Callback function after ESP reboot command
 *****************************************************************/
function CallRebootPage(json) {
  addLogLine("ESP Rebooting...");
  setTimeout(() => {
    ws.close();
  }, 5000);
}

/*****************************************************************
 * Callback function to handle incoming log data
 *****************************************************************/
function MyCallback(json) {
  if (json && json.logline !== undefined) {
    window.addLogLine(json.logline);
  }
}

/*****************************************************************
 * Add a log line to the log area
 *****************************************************************/
window.addLogLine = function(line) {
      logLineCount++;
      logRawLines.push(line);

      const logLinesDiv = document.getElementById('logLines');
      // Create row number
      const rowDiv = document.createElement('div');
      rowDiv.className = 'log-row';

      const rowNum = document.createElement('span');
      rowNum.className = 'log-row-number';
      rowNum.textContent = logLineCount;

      const rowContent = document.createElement('span');
      rowContent.className = 'log-row-content';
      rowContent.textContent = line;

      rowDiv.appendChild(rowNum);
      rowDiv.appendChild(rowContent);

      logLinesDiv.appendChild(rowDiv);

      // Scroll to bottom
      logLinesDiv.scrollTop = logLinesDiv.scrollHeight;
};

/*****************************************************************
 * Clear log
 *****************************************************************/
export function clearLog() {
  logLineCount = 0;
  logRawLines = [];
  document.getElementById('logLines').innerHTML = '';
}

/*****************************************************************
 * Download log as text file (without row numbers, only original lines)
 *****************************************************************/
export function downloadLog() {
    const text = logRawLines.join('\n');
    const blob = new Blob([text], {type: "text/plain"});
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'webserial_log.txt';
    document.body.appendChild(a);
    a.click();
    setTimeout(() => {
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    }, 100);
}

/*****************************************************************
 * ESP Reboot via WebSocket and call reboot callback function
 *****************************************************************/
export function doReboot() {
  var data = {};
    data['cmd'] = {};
    data['cmd']['action'] = "reboot";
    data['cmd']['callbackFn'] = "CallRebootPage";
    requestData(data);
}

/*****************************************************************
 * Update WebSocket status indicator
 *****************************************************************/
function updateWsStatus() {
  const statusElem = document.getElementById('ws-status');
  const icon = statusElem.querySelector('.fa');
  if (!statusElem || !icon) return;

  if (ws && ws.readyState === WebSocket.OPEN) {
    statusElem.classList.add('online');
    statusElem.classList.remove('offline');
    icon.className = 'fa fa-link';
    statusElem.title = "WebSocket online";
  } else {
    window.addLogLine("websocket connection goes offline ...");
    statusElem.classList.add('offline');
    statusElem.classList.remove('online');
    icon.className = 'fa fa-chain-broken';
    statusElem.title = "WebSocket offline";
  }
}

/*****************************************************************
 * Change log font size
 *****************************************************************/
export function changeLogFontSize(delta) {
  const logLines = document.getElementById('logLines');
  if (!logLines) return;

  // Aktuelle Schriftgröße auslesen (z.B. "16px" -> 16)
  const style = window.getComputedStyle(logLines, null).getPropertyValue('font-size');
  let fontSize = parseFloat(style);

  // Neue Schriftgröße berechnen und begrenzen
  fontSize = Math.max(8, Math.min(32, fontSize + delta));

  // Anwenden auf Logausgabe
  logLines.style.fontSize = fontSize + 'px';

  // Auch auf alle Zeilennummern anwenden (z.B. 95% der Loggröße)
  const rowNumbers = logLines.querySelectorAll('.log-row-number');
  rowNumbers.forEach(span => {
    span.style.fontSize = (fontSize * 0.95) + 'px';
  });
};