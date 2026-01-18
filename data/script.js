document.addEventListener('DOMContentLoaded', () => {
    const timerDisplay = document.getElementById('timer-display');
    const timerSubtitle = document.getElementById('timer-subtitle');
    const historyBody = document.getElementById('history-body');

    let lapStartTime = 0;
    let timerIntervalId = null;
    let isTiming = false;

    function formatTime(timeInSeconds) {
        if (typeof timeInSeconds !== 'number' || isNaN(timeInSeconds) || timeInSeconds < 0) {
            return "00:00.000";
        }
        const date = new Date(timeInSeconds * 1000);
        const minutes = date.getUTCMinutes();
        const seconds = date.getUTCSeconds();
        const milliseconds = date.getUTCMilliseconds();
        return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}.${String(milliseconds).padStart(3, '0')}`;
    }

    function startLocalTimer(serverRaceTime) {
        if (isTiming) { // If already timing, just recalibrate
            lapStartTime = Date.now() - (serverRaceTime * 1000);
            return;
        }
        
        isTiming = true;
        lapStartTime = Date.now() - (serverRaceTime * 1000);

        timerSubtitle.textContent = 'GO';
        timerSubtitle.className = 'timer-subtitle status-go';
        
        if (timerIntervalId) clearInterval(timerIntervalId);
        timerIntervalId = setInterval(() => {
            const elapsedTime = (Date.now() - lapStartTime) / 1000;
            timerDisplay.textContent = formatTime(elapsedTime);
        }, 50);
    }

    function stopLocalTimer() {
        if (!isTiming) return;
        isTiming = false;
        clearInterval(timerIntervalId);
        timerIntervalId = null;
        lapStartTime = 0;
    }

    function updateData() {
        fetch('/api/v1/data')
            .then(response => response.json())
            .then(data => {
                if (data.race_time > 0) {
                    startLocalTimer(data.race_time);
                } else {
                    stopLocalTimer();
                    if (data.value > 0) {
                        timerDisplay.textContent = formatTime(data.value);
                        timerSubtitle.textContent = 'last lap';
                        timerSubtitle.className = 'timer-subtitle status-last-lap';
                    } else {
                        timerDisplay.textContent = "00:00.000";
                        timerSubtitle.textContent = 'ready to go';
                        timerSubtitle.className = 'timer-subtitle';
                    }
                }

                // Update history
                historyBody.innerHTML = '';
                if (data.history && data.history.length > 0) {
                    data.history.slice().reverse().forEach((lap, index) => {
                        if (lap.value > 0) {
                            const row = historyBody.insertRow();
                            const lapCell = row.insertCell(0);
                            const timeCell = row.insertCell(1);
                            lapCell.textContent = data.history.length - index;
                            timeCell.textContent = formatTime(lap.value);
                        }
                    });
                }
                
                // Update battery info
                const rxBattery = document.querySelector('.battery-info.rx');
                const txBattery = document.querySelector('.battery-info.tx');
                if(rxBattery) rxBattery.textContent = `RX: ${data.battery}%`;
                if(txBattery) txBattery.textContent = `TX: ${data.tx_battery >= 0 ? data.tx_battery + '%' : '---'}`;

            })
            .catch(err => {
                console.error('Error fetching data:', err);
                timerSubtitle.textContent = "connection error";
                stopLocalTimer();
            });
    }

    setInterval(updateData, 300);
});

function changeMode(mode) {
    fetch(`/api/v1/mode?m=${mode}`)
        .then(() => {
            location.reload();
        })
        .catch(err => console.error('Error changing mode:', err));
}

function resetMeasurements() {
    fetch('/api/v1/reset', { method: 'POST' })
        .then(() => {
            document.getElementById('timer-display').textContent = "00:00.000";
            document.getElementById('timer-subtitle').textContent = 'ready to go';
            document.getElementById('timer-subtitle').className = 'timer-subtitle';
            document.getElementById('history-body').innerHTML = '';
            // The stopLocalTimer() will be called on the next data update.
        })
        .catch(err => console.error('Error resetting:', err));
}
