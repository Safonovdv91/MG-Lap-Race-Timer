let currentMode = 0;
let historyData = [];

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

      if (currentMode == 0) { // Speedometer
        valueDisplay.textContent = data.currentValue.toFixed(2);
        unitDisplay.textContent = 'km/h';
        historyData = data.speedHistory;
      } else { // Timer modes
        valueDisplay.textContent = data.currentValue.toFixed(3);
        unitDisplay.textContent = 's';
        historyData = data.lapHistory;
      }

      // Обновление таблицы истории
      const tableBody = document.getElementById('historyBody');
      tableBody.innerHTML = '';

      for (let i = 0; i < historyData.length; i++) {
        if (historyData[i].value > 0) {
          const row = document.createElement('tr');

          const indexCell = document.createElement('td');
          indexCell.textContent = i + 1;
          row.appendChild(indexCell);

          // Значение
          const valueCell = document.createElement('td');
          valueCell.textContent = currentMode == 0 ?
            historyData[i].value.toFixed(2) + ' km/h' :
            historyData[i].value.toFixed(3) + ' s';
          row.appendChild(valueCell);

          // Время
          const timeCell = document.createElement('td');
          const date = new Date(historyData[i].time);
          timeCell.textContent = date.toLocaleTimeString();
          row.appendChild(timeCell);

          tableBody.appendChild(row);
        }
      }
    });
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
