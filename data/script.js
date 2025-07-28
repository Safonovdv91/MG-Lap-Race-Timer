let currentMode = 0;
let historyData = [];
let raceTimerInterval = null;
let lastRaceTime = 0;
let raceStartTime = 0;
let isRaceActive = false;

function updateDisplay() {
  fetch('/data')
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
    });
}

// Форматирование времени в mm:ss.ms
function formatTime(seconds) {
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  const ms = Math.floor((seconds % 1) * 1000);
  return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}.${ms.toString().padStart(3, '0')}`;
}

// Форматирование даты/времени
function formatDateTime(timestamp) {
  const date = new Date(timestamp);
  return date.toLocaleTimeString();
}

function changeMode() {
  const mode = document.getElementById('modeSelect').value;
  fetch('/mode?value=' + mode)
    .then(() => updateDisplay());
}


function reset() {
  fetch('/reset')
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
  fetch('/distance?value=' + distance)
    .then(() => {
      // Не обновляем весь дисплей, только значение
      document.getElementById('valueDisplay').textContent = 
        (currentMode == 0 ? data.currentValue.toFixed(2) : data.currentValue.toFixed(3));
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
