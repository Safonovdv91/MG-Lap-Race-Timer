#include "web_content.h"

String getWifiSettingsContent() {
  return   R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>WiFi Settings</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 500px; margin: 0 auto; }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; }
    input { width: 100%; padding: 8px; box-sizing: border-box; }
    button { background: #4CAF50; color: white; border: none; padding: 10px 15px; cursor: pointer; }
  </style>
</head>
<body>
  <div class="container">
    <h1>WiFi Settings</h1>
    <form action="/updatewifi" method="post">
      <div class="form-group">
        <label for="ssid">Network Name (SSID):</label>
        <input type="text" id="ssid" name="ssid" value="%SSID%" required>
      </div>
      <div class="form-group">
        <label for="password">New Password:</label>
        <input type="password" id="password" name="password" required>
      </div>
      <button type="submit">Save Settings</button>
    </form>
  </div>
</body>
</html>
)rawliteral";
}


String getHTMLContent() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta name="theme-color" content="#FF4F00">
  <title>ESP32 Speedometer</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
<div id="batteryInfo" style="position: fixed; top: 10px; right: 10px; z-index: 100;"></div>
  <div class="container">
    <h1>MG Race/Lap/Speed timer</h1>
    
    <div class="controls">
      <select id="modeSelect" onchange="changeMode()">
        <option value="0">Speedometer (Требуется 2 датчика)</option>
        <option value="1">Lap Timer (Один датчик)</option>
        <option value="2">Race Timer (Требуется 2 датчика)</option>
      </select>
      <button onclick="window.location.href='/wifisettings'">WiFi Settings</button>
      
    <div id="distanceControl" class="distance-control">
      <label>Distance (m):</label>
      <button onclick="changeDistance(-0.1)">-0.1</button>
      <input type="number" id="distanceInput" step="0.1" min="0.1" value="10.0">
      <button onclick="changeDistance(0.1)">+0.1</button>
    </div>
      
      <button onclick="reset()">Reset</button>
    </div>
    
    <div class="display">
      <div id="valueDisplay" class="main-value time-display">0.00</div>
      <div id="unitDisplay" class="unit">km/h</div>
    </div>
    <div id="sensorStatus" class="sensor-status">
      <span id="sensor1Status">Д1: --- </span>
      <span id="sensor2Status">Д2: --- </span>
    </div>
    <div class="history">
      <h2>История проездов</h2>
      <table id="historyTable">
        <thead>
          <tr>
            <th>#</th>
            <th>Value</th>
            <th>Time</th>
          </tr>
        </thead>
        <tbody id="historyBody"></tbody>
      </table>
    </div>
  </div>
  
  <script src="/script.js"></script>
</body>
</html>
)rawliteral";
}