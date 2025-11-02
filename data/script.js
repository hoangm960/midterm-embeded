const TEMP_WARN = 27;
const TEMP_CRIT = 30;
const HUM_WARN = 60;
const HUM_CRIT = 90;

const fanBtn = document.getElementById('fan-btn');
const exitBtn = document.getElementById('exit-btn');

function toggleDevice(name) {
    fetch('/toggle?device=' + name)
        .then(r => r.json())
        .then(j => {
            document.getElementById('fan-status').innerText = j.fan;
            document.getElementById('exit-status').innerText = j.exit;
        });
}

function showAlert(type, msg) {
    const box = document.getElementById('alert-box');
    if (type === 'none') {
        box.classList.add('hidden');
        box.classList.remove('warning', 'critical');
        return;
    }
    box.innerText = msg;
    box.classList.remove('hidden', 'warning', 'critical');
    box.classList.add(type);
}

function updateBar(id, value, warn, crit, max) {
    const bar = document.getElementById(id);
    const pct = Math.min((value / max) * 100, 100);
    bar.style.width = pct + '%';
    bar.classList.remove('warning', 'critical');
    if (value >= crit) bar.classList.add('critical');
    else if (value >= warn) bar.classList.add('warning');
}

function setThresholdMarkers() {
    const setMarker = (id, warn, crit, max) => {
        document.getElementById(`${id}-warn-marker`).style.left = (warn / max * 100) + '%';
        document.getElementById(`${id}-crit-marker`).style.left = (crit / max * 100) + '%';
    };

    setMarker('temp', TEMP_WARN, TEMP_CRIT, 100);
    setMarker('hum', HUM_WARN, HUM_CRIT, 100);
}

window.addEventListener('DOMContentLoaded', setThresholdMarkers);

function updateControls(isCritical) {
    fanBtn.disabled = !isCritical;
    exitBtn.disabled = !isCritical;
}

setInterval(() => {
    fetch('/sensors')
        .then(res => res.json())
        .then(d => {
            // Update displayed values
            document.getElementById('temp-val').innerText = d.temp.toFixed(1);
            document.getElementById('hum-val').innerText = d.hum.toFixed(1);

            // Update progress bars
            let tempPercent = (d.temp / 100) * 100;
            let humPercent = (d.hum / 100) * 100;
            const tempBar = document.getElementById('temp-bar');
            const humBar = document.getElementById('hum-bar');

            tempBar.style.width = Math.min(tempPercent, 100) + '%';
            humBar.style.width = Math.min(humPercent, 100) + '%';

            // Determine state
            let isCritical = false;

            // Temperature thresholds
            if (d.temp >= TEMP_CRIT) {
                tempBar.className = 'progress-bar critical';
                isCritical = true;
            } else if (d.temp >= TEMP_WARN) {
                tempBar.className = 'progress-bar warning';
            } else {
                tempBar.className = 'progress-bar';
            }

            // Humidity thresholds
            if (d.hum >= HUM_CRIT) {
                humBar.className = 'progress-bar critical';
                isCritical = true;
            } else if (d.hum >= HUM_WARN) {
                humBar.className = 'progress-bar warning';
            } else {
                humBar.className = 'progress-bar';
            }

            // Lock/unlock controls
            updateControls(isCritical);

            const overlay = document.getElementById('alert-overlay');

            if (isCritical) {
                overlay.classList.add('active');
            } else {
                overlay.classList.remove('active');
            }
        });
}, 3000);