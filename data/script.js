let currentMode = 0;
let historyData = [];

function updateDisplay() {
  fetch('/data')
    .then(response => response.json())
    .then(data => {
      currentMode = data.mode;
      document.getElementById('modeSelect').value = currentMode;
      document.getElementById('distanceInput').value = data.distance;

      // Обновление главного дисплея
      const valueDisplay = document.getElementById('valueDisplay');
      if (data.measurementInProgress) { // Добавим этот флаг в JSON
        valueDisplay.innerHTML = `<span class="active-measurement">${formatTime(data.currentValue)}</span>`;
      } else {
        valueDisplay.textContent = currentMode == 0 ? 
          data.currentValue.toFixed(2) : 
          formatTime(data.currentValue);
      }
      const unitDisplay = document.getElementById('unitDisplay');

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

// Update every 300ms
setInterval(updateDisplay, 300);
updateDisplay();
