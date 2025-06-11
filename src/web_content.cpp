// web_content.cpp
#include "web_content.h"

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
  <div class="container">
    <h1>MG Race/Lap/Speed timer</h1>
    
    <div class="controls">
      <select id="modeSelect" onchange="changeMode()">
        <option value="0">Speedometer (Требуется 2 датчика)</option>
        <option value="1">Lap Timer (Один датчик)</option>
        <option value="2">Race Timer (Требуется 2 датчика)</option>
      </select>
      
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