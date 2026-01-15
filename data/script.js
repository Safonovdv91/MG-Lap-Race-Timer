let currentMode = 0;
let historyData = [];
let raceTimerInterval = null;
let lastRaceTime = 0;
let raceStartTime = 0;
let isRaceActive = false;

function updateBatteryInfo(voltage, percentage) {
  const batteryInfo = document.getElementById('batteryInfo');
  
  let html = `
    <div style="display: flex; align-items: center; gap: 5px;">
      <div style="position: relative; width: 24px; height: 12px; border: 1px solid #ccc;">
        <div style="position: absolute; 
                    height: 100%; 
                    width: ${percentage}%; 
                    background: ${percentage > 20 ? '#4CAF50' : '#F44336'};">
        </div>
      </div>
      <div style="font-size: 12px;">
        ${voltage}V (${percentage}%)
      </div>
    </div>
  `;
  
  batteryInfo.innerHTML = html;
}

function updateDisplay() {
  fetch('api/v1/data')
    .then(response => response.json())
    .then(data => {
      updateSensorStatus(data.sensor1Active, data.sensor2Active);
      currentMode = data.mode;
      document.getElementById('modeSelect').value = currentMode;
      document.getElementById('distanceInput').value = data.distance;

      // Обработка разных режимов
      const valueDisplay = document.getElementById('valueDisplay');
      const unitDisplay = document.getElementById('unitDisplay');

      // Подсветка режима измерения
      if (data.measurementInProgress) { // Добавим этот флаг в JSON
        valueDisplay.innerHTML = `<span class="active-measurement">${formatTime(data.currentTime)}</span>`;
      } else {
        valueDisplay.textContent = currentMode == 0 ? 
          data.currentValue.toFixed(2) : 
          formatTime(data.currentValue);
      }
      if (currentMode == 0) { // Speedometer
        valueDisplay.textContent = data.currentValue.toFixed(2);
        unitDisplay.textContent = 'km/h';
        historyData = data.speedHistory;
      } else { // Timer modes
        unitDisplay.textContent = 'mm:ss.ms';
        historyData = data.lapHistory;
      }

      // Обновление таблицы истории
      const tableBody = document.getElementById('historyBody');
      tableBody.innerHTML = '';

      for (let i = 0; i < historyData.length; i++) {
        const record = currentMode == 0 ? data.speedHistory[i] : data.lapHistory[i];
        if (record && record.value > 0) {
          const row = document.createElement('tr');

          const indexCell = document.createElement('td');
          indexCell.textContent = i + 1;
          row.appendChild(indexCell);

          // Значение
          const valueCell = document.createElement('td');
          valueCell.innerHTML = currentMode == 0 ? 
            `<strong>${record.value.toFixed(2)} km/h</strong>` : 
            `<strong>${formatTime(record.value)}</strong>`;
          row.appendChild(valueCell);

          tableBody.appendChild(row);
        }
      }
      updateBatteryInfo(data.batteryVoltage, data.batteryPercentage);
    });
}

// Форматирование времени в mm:ss.cc (минуты:секунды.сотые_доли_секунды)
function formatTime(seconds) {
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  const cs = Math.floor((seconds % 1) * 100); // Сотые доли секунды
  return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}.${cs.toString().padStart(2, '0')}`;
}

// Форматирование даты/времени
function formatDateTime(timestamp) {
  const date = new Date(timestamp);
  return date.toLocaleTimeString();
}

function changeMode() {
  const mode = document.getElementById('modeSelect').value;
  fetch('/api/v1/mode?value=' + mode)
    .then(() => updateDisplay());
}


function reset() {
  fetch('/api/v1/reset')
    .then(() => updateDisplay());
}

function changeDistance(step) {
  const input = document.getElementById('distanceInput');
  let value = parseFloat(input.value) + step;
  
  // Ограничиваем минимальное значение
  if (value < 0.1) value = 0.1;
  
  input.value = value.toFixed(1);
  updateDistance(); // Автоматически отправляем новое значение
}

function updateDistance() {
  const distance = document.getElementById('distanceInput').value;
  fetch('/api/v1/distance?value=' + distance)
    .then(() => {
      // Обновляем весь дисплей, чтобы получить актуальные данные
      updateDisplay();
    });
}

document.querySelectorAll('.distance-control button').forEach(btn => {
  btn.addEventListener('touchstart', function(e) {
    e.preventDefault();
    this.click();
  });
});

// Функцию для обновления статуса датчиков
function updateSensorStatus(sensor1Active, sensor2Active) {
  const sensor1Elem = document.getElementById('sensor1Status');
  const sensor2Elem = document.getElementById('sensor2Status');
  
  sensor1Elem.textContent = 'Д1: ' + (sensor1Active ? 'АКТИВЕН' : '---');
  sensor2Elem.textContent = 'Д2: ' + (sensor2Active ? 'АКТИВЕН' : '---');
  
  sensor1Elem.className = sensor1Active ? 'sensor-active' : '';
  sensor2Elem.className = sensor2Active ? 'sensor-active' : '';
}


// Update every 300ms
setInterval(updateDisplay, 1000);
updateDisplay();
