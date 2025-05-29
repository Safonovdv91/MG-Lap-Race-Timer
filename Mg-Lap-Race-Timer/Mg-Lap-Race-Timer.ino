#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

// Конфигурация
#define SENSOR1_PIN 13
#define SENSOR2_PIN 12
#define DEBOUNCE_TIME 50
#define MIN_LAP_TIME 3000000
#define HISTORY_SIZE 5

WebServer server(80);

enum Mode { SPEEDOMETER, LAP_TIMER, RACE_TIMER };
Mode currentMode = SPEEDOMETER;
float distance = 10.0;

struct Measurement {
  float value;
  unsigned long timestamp;
};

Measurement speedHistory[HISTORY_SIZE];
Measurement lapHistory[HISTORY_SIZE];
int historyIndex = 0;
volatile unsigned long startTime = 0;
volatile unsigned long endTime = 0;
volatile bool sensor1Triggered = false;
volatile bool sensor2Triggered = false;
volatile bool measurementReady = false;
volatile float currentValue = 0.0;

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Motogymkhana Tool Kit (Speedometer, Lap/Race Timer)</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: #f5f5f5;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    .controls {
      margin: 20px 0;
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
    }
    .display {
      text-align: center;
      margin: 30px 0;
    }
    .main-value {
      font-size: 4rem;
      font-weight: bold;
      color: #2c3e50;
    }
    .unit {
      font-size: 1.5rem;
      color: #7f8c8d;
    }
    .timer-status {
      font-size: 1.2em;
      color: #e74c3c;
      margin: 10px 0;
      height: 1.5em;
    }
    @keyframes pulse {
      0% { opacity: 1; }
      50% { opacity: 0.5; }
      100% { opacity: 1; }
    }
    .timer-status.running {
      animation: pulse 1.5s infinite;
      color: #e74c3c;
      font-weight: bold;
    }
    .history {
      margin-top: 30px;
    }
    table {
      width: 100%;
      border-collapse: collapse;
    }
    th, td {
      padding: 10px;
      text-align: left;
      border-bottom: 1px solid #ddd;
    }
    th {
      background-color: #f2f2f2;
    }
    button, select, input {
      padding: 8px 12px;
      border-radius: 4px;
      border: 1px solid #ddd;
    }
    button {
      background-color: #3498db;
      color: white;
      border: none;
      cursor: pointer;
    }
    button:hover {
      background-color: #2980b9;
    }
    .distance-control {
      display: flex;
      align-items: center;
      gap: 5px;
    }
    #distanceInput {
      width: 60px;
    }
    @media (max-width: 480px) {
      .main-value {
        font-size: 3rem;
      }
      .controls {
        flex-direction: column;
        align-items: stretch;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Motogymkhana Tool Kit (Speedometer, Lap/Race Timer)</h1>
    
    <div class="controls">
      <select id="modeSelect" onchange="changeMode()">
        <option value="0">Speedometer (Требуется 2 датчика)</option>
        <option value="1">Lap Timer</option>
        <option value="2">Race Timer (Требуется 2 датчика)</option>
      </select>
      
      <div id="distanceControl" class="distance-control">
        <label>Distance (m):</label>
        <input type="number" id="distanceInput" step="0.1" min="0.1" value="10.0">
        <button onclick="updateDistance()">Update</button>
      </div>
      
      <button onclick="reset()">Reset</button>
    </div>
    
    <div class="display">
      <div id="valueDisplay" class="main-value">0.00</div>
      <div id="unitDisplay" class="unit">km/h</div>
      <div class="timer-status" id="timerStatus"></div>
    </div>
    
    <div class="history">
      <h2>History</h2>
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
  
  <script>
    let currentMode = 0;
    let historyData = [];
    let timerStatus = document.getElementById('timerStatus');

      function formatTime(seconds) {
        if(isNaN(seconds)) return "00:00.000";
        const mins = Math.floor(seconds / 60);
        const secs = (seconds % 60).toFixed(3);
        return `${mins.toString().padStart(2, '0')}:${secs.padStart(6, '0')}`;
      }


    function updateDisplay() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          currentMode = data.mode;
          document.getElementById('modeSelect').value = currentMode;
          document.getElementById('distanceInput').value = data.distance;
          
          // Update main display
          const valueDisplay = document.getElementById('valueDisplay');
          const unitDisplay = document.getElementById('unitDisplay');
          
          if (currentMode == 0) {
            valueDisplay.textContent = data.currentValue.toFixed(2);
            unitDisplay.textContent = 'km/h';
            historyData = data.speedHistory;
            timerStatus.textContent = "";
            timerStatus.className = "timer-status";
          } else {
            valueDisplay.textContent = formatTime(data.currentValue);
            unitDisplay.textContent = '';
            historyData = data.lapHistory;
            
            if (data.sensor1Triggered && !data.sensor2Triggered) {
              timerStatus.textContent = "⏱ Timer running...";
              timerStatus.className = "timer-status running";
            } else {
              timerStatus.textContent = data.currentValue > 0 ? "✔ Ready" : "Waiting...";
              timerStatus.className = "timer-status";
            }
          }
          
          // Update history table
          const tableBody = document.getElementById('historyBody');
          tableBody.innerHTML = '';
          
          for (let i = 0; i < historyData.length; i++) {
            if (historyData[i].value > 0) {
              const row = document.createElement('tr');
              
              const indexCell = document.createElement('td');
              indexCell.textContent = i + 1;
              row.appendChild(indexCell);
              
              const valueCell = document.createElement('td');
              valueCell.textContent = currentMode == 0 ? 
                historyData[i].value.toFixed(2) + ' km/h' : 
                formatTime(historyData[i].value);
              row.appendChild(valueCell);
              
              const timeCell = document.createElement('td');
              const date = new Date(historyData[i].time);
              timeCell.textContent = date.toLocaleTimeString();
              row.appendChild(timeCell);
              
              tableBody.appendChild(row);
            }
          }
        })
        .catch(error => console.error('Error:', error));
    }

    function changeMode() {
      const mode = document.getElementById('modeSelect').value;
      fetch('/mode?value=' + mode)
        .then(() => updateDisplay());
    }

    function updateDistance() {
      const distance = document.getElementById('distanceInput').value;
      fetch('/distance?value=' + distance)
        .then(() => updateDisplay());
    }

    function reset() {
      fetch('/reset')
        .then(() => updateDisplay());
    }

    // Update every 300ms
    setInterval(updateDisplay, 300);
    updateDisplay();
  </script>
</body>
</html>
)rawliteral";

void IRAM_ATTR handleSensor1() {
  static unsigned long lastInterrupt = 0;
  unsigned long now = micros();
  
  if (now - lastInterrupt < DEBOUNCE_TIME * 1000) return;
  lastInterrupt = now;

  if (currentMode == SPEEDOMETER) {
    if (!sensor1Triggered && !sensor2Triggered) {
      startTime = now;
      sensor1Triggered = true;
    }
  } 
  else if (currentMode == LAP_TIMER) {
    if (!sensor1Triggered) {
      startTime = now;
      sensor1Triggered = true;
    } else if (now - startTime > MIN_LAP_TIME) {
      endTime = now;
      sensor1Triggered = false;
      measurementReady = true;
    }
  }
  else if (currentMode == RACE_TIMER) {
    if (!sensor1Triggered) {
      startTime = now;
      sensor1Triggered = true;
    }
  }
}

void IRAM_ATTR handleSensor2() {
  static unsigned long lastInterrupt = 0;
  unsigned long now = micros();
  
  if (now - lastInterrupt < DEBOUNCE_TIME * 1000) return;
  lastInterrupt = now;

  if (currentMode == SPEEDOMETER) {
    if (sensor1Triggered && !sensor2Triggered) {
      endTime = now;
      sensor2Triggered = true;
      measurementReady = true;
    }
  }
  else if (currentMode == RACE_TIMER) {
    if (sensor1Triggered) {
      endTime = now;
      sensor1Triggered = false;
      measurementReady = true;
    }
  }
}

void addToHistory(Measurement history[], float value) {
  history[historyIndex % HISTORY_SIZE] = {value, millis()};
  historyIndex++;
}

void processMeasurements() {
  if (measurementReady) {
    unsigned long duration = endTime - startTime;
    
    if (currentMode == SPEEDOMETER) {
      currentValue = (distance / (duration / 1000000.0)) * 3.6;
      addToHistory(speedHistory, currentValue);
    } else {
      currentValue = duration / 1000000.0;
      addToHistory(lapHistory, currentValue);
    }
    
    measurementReady = false;
    sensor1Triggered = false;
    sensor2Triggered = false;
  }
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleData() {
  String json = "{";
  json += "\"mode\":" + String(currentMode) + ",";
  json += "\"currentValue\":" + String(currentValue) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"sensor1Triggered\":" + String(sensor1Triggered ? "true" : "false") + ",";
  json += "\"sensor2Triggered\":" + String(sensor2Triggered ? "true" : "false") + ",";
  
  json += "\"speedHistory\":[";
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (i > 0) json += ",";
    json += "{\"value\":" + String(speedHistory[i].value) + ",\"time\":" + String(speedHistory[i].timestamp) + "}";
  }
  json += "],";
  
  json += "\"lapHistory\":[";
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (i > 0) json += ",";
    json += "{\"value\":" + String(lapHistory[i].value) + ",\"time\":" + String(lapHistory[i].timestamp) + "}";
  }
  json += "]";
  
  json += "}";
  server.send(200, "application/json", json);
}

void handleReset() {
  startTime = 0;
  endTime = 0;
  sensor1Triggered = false;
  sensor2Triggered = false;
  currentValue = 0.0;
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  if (server.hasArg("value")) {
    currentMode = (Mode)server.arg("value").toInt();
    startTime = 0;
    endTime = 0;
    sensor1Triggered = false;
    sensor2Triggered = false;
    measurementReady = false;
    currentValue = 0.0;
  }
  server.send(200, "text/plain", "OK");
}

void handleDistance() {
  if (server.hasArg("value")) {
    distance = server.arg("value").toFloat();
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  
  pinMode(SENSOR1_PIN, INPUT_PULLUP);
  pinMode(SENSOR2_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR1_PIN), handleSensor1, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR2_PIN), handleSensor2, FALLING);

  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  WiFi.softAP("ESP32-Speedometer", "12345678");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/reset", handleReset);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/distance", HTTP_GET, handleDistance);
  server.begin();
}

void loop() {
  server.handleClient();
  processMeasurements();
}
