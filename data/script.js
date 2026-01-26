document.addEventListener('DOMContentLoaded', () => {
    const timerDisplay = document.getElementById('timer-display');
    const timerSubtitle = document.getElementById('timer-subtitle');
    const historyBody = document.getElementById('history-body');
    const statusIndicator = document.getElementById('timer-status');

    let gateway = `ws://${window.location.hostname}:81/`;
    let websocket;

    // --- Local Timer for smooth animation ---
    let localTimer = {
        intervalId: null,
        isTiming: false,
        startTime: 0,
        
        start: function(serverRaceTime) {
            if (this.isTiming) { // If already timing, just recalibrate
                this.startTime = Date.now() - (serverRaceTime * 1000);
                return;
            }
            this.isTiming = true;
            this.startTime = Date.now() - (serverRaceTime * 1000);
            if (this.intervalId) clearInterval(this.intervalId);
            this.intervalId = setInterval(() => {
                const elapsedTime = (Date.now() - this.startTime) / 1000;
                timerDisplay.textContent = formatTime(elapsedTime);
            }, 50);
        },

        stop: function() {
            if (!this.isTiming) return;
            this.isTiming = false;
            clearInterval(this.intervalId);
            this.intervalId = null;
            this.startTime = 0;
        }
    };

    function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen    = onOpen;
        websocket.onclose   = onClose;
        websocket.onmessage = onMessage;
    }

    function onOpen(event) {
        console.log('Connection opened');
        timerSubtitle.textContent = 'connected';
        timerSubtitle.className = 'timer-subtitle';
    }

    function onClose(event) {
        console.log('Connection closed');
        timerSubtitle.textContent = 'connection lost';
        timerSubtitle.className = 'timer-subtitle status-last-lap'; // Use orange for error
        statusIndicator.className = '';
        localTimer.stop();
        setTimeout(initWebSocket, 2000); // Try to reconnect every 2 seconds
    }

    function onMessage(event) {
        let data = JSON.parse(event.data);
        
        const status = data.timer_status || 'ready';
        statusIndicator.className = 'status-' + status;

        // Update main timer display based on state
        if (status === 'running') {
            localTimer.start(data.race_time);
            timerSubtitle.textContent = 'GO';
            timerSubtitle.className = 'timer-subtitle status-go';
        } else { // Covers 'ready' and 'display'
            localTimer.stop();
            
            if (data.value > 0) {
                timerDisplay.textContent = formatTime(data.value);
            } else {
                timerDisplay.textContent = "00:00.000";
            }

            if (status === 'display') {
                timerSubtitle.textContent = 'last lap';
                timerSubtitle.className = 'timer-subtitle status-last-lap';
            } else { // ready
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
    }

    initWebSocket();
});

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

function changeMode(mode) {
    fetch(`/api/v1/mode?m=${mode}`)
        .then(() => {
            location.reload();
        })
        .catch(err => console.error('Error changing mode:', err));
}

function resetMeasurements() {
    fetch('/api/v1/reset', { method: 'POST' })
        .catch(err => console.error('Error resetting:', err));
    // The UI will update automatically via the broadcast triggered by the reset
}
