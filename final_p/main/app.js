// ======================
// Variables globales
// ======================
let seconds         = null;
let otaTimerVar     = null;
let wifiConnectInterval = null;

// ======================
// Inicialización
// ======================
document.addEventListener('DOMContentLoaded', () => {
    console.log("ready (sin jQuery)");

    // Si aún no tienes OTA / DHT implementados puedes comentar estas dos:
    getUpdateStatus();
    startDHTSensorInterval();

    const btnToggle = document.getElementById("toogle_led");
    const btnUart   = document.getElementById("apagar_uart");

    if (btnToggle) {
        btnToggle.addEventListener("click", toogle_led);
    }
    if (btnUart) {
        btnUart.addEventListener("click", turn_off_uart);
    }

    // ⏰ Actualizar la hora al cargar y luego cada 1 s
    updateTime();
    setInterval(updateTime, 1000);
});

// ======================
// OTA: info del archivo
// ======================
function getFileInfo() {
    const x = document.getElementById("selected_file");
    const file = x.files[0];

    document.getElementById("file_info").innerHTML =
        "<h4>File: " + file.name + "<br>" +
        "Size: " + file.size + " bytes</h4>";
}

// ======================
// OTA: subir firmware
// ======================
function updateFirmware() {
    const formData  = new FormData();
    const fileSelect = document.getElementById("selected_file");

    if (fileSelect.files && fileSelect.files.length === 1) {
        const file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML =
            "Uploading " + file.name + ", Firmware Update in Progress...";

        const request = new XMLHttpRequest();
        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);
    } else {
        window.alert('Select A File First');
    }
}

function updateProgress(oEvent) {
    if (oEvent.lengthComputable) {
        getUpdateStatus();
    } else {
        window.alert('total size is unknown');
    }
}

function getUpdateStatus() {
    const xhr = new XMLHttpRequest();
    const requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false); // síncrono, como tenías
    xhr.send('ota_update_status');

    if (xhr.readyState === 4 && xhr.status === 200) {
        const response = JSON.parse(xhr.responseText);

        const latest = document.getElementById("latest_firmware");
        if (latest) {
            latest.innerHTML = response.compile_date + " - " + response.compile_time;
        }

        if (response.ota_update_status === 1) {
            seconds = 10;
            otaRebootTimer();
        } else if (response.ota_update_status === -1) {
            document.getElementById("ota_update_status").innerHTML =
                "!!! Upload Error !!!";
        }
    }
}

function otaRebootTimer() {
    document.getElementById("ota_update_status").innerHTML =
        "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds === 0) {
        clearTimeout(otaTimerVar);
        window.location.reload();
    } else {
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}

// ======================
// DHT22
// ======================
function getDHTSensorValues() {
    fetch('/dhtSensor.json', { cache: 'no-store' })
        .then(resp => {
            if (!resp.ok) throw new Error("HTTP " + resp.status);
            return resp.json();
        })
        .then(data => {
            const t = document.getElementById("temperature_reading");
            const h = document.getElementById("humidity_reading");
            if (t) t.textContent = data["temp"];
            if (h) h.textContent = data["humidity"];
        })
        .catch(err => {
            console.error("Error leyendo /dhtSensor.json:", err);
        });
}

function startDHTSensorInterval() {
    setInterval(getDHTSensorValues, 5000);
}

// ======================
// Estado WiFi
// ======================
function stopWifiConnectStatusInterval() {
    if (wifiConnectInterval != null) {
        clearInterval(wifiConnectInterval);
        wifiConnectInterval = null;
    }
}

function getWifiConnectStatus() {
    const xhr = new XMLHttpRequest();
    const requestURL = "/wifiConnectStatus";
    xhr.open('POST', requestURL, false);
    xhr.send('wifi_connect_status');

    if (xhr.readyState === 4 && xhr.status === 200) {
        const response = JSON.parse(xhr.responseText);

        const div = document.getElementById("wifi_connect_status");
        if (!div) return;

        div.innerHTML = "Connecting...";

        if (response.wifi_connect_status === 2) {
            div.innerHTML =
                "<h4 class='rd'>Failed to Connect. Please check your AP credentials and compatibility</h4>";
            stopWifiConnectStatusInterval();
        } else if (response.wifi_connect_status === 3) {
            div.innerHTML =
                "<h4 class='gr'>Connection Success!</h4>";
            stopWifiConnectStatusInterval();
        }
    }
}

function startWifiConnectStatusInterval() {
    wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

// ======================
// UART
// ======================
function turn_off_uart() {
    fetch('/uart_off.json', {
        method: 'POST',
        cache: 'no-store'
    }).catch(err => console.error("Error /uart_off.json:", err));
}

// ======================
// Toggle LED
// ======================
function toogle_led() {
    fetch('/toogle_led.json', {
        method: 'POST',
        cache: 'no-store'
    }).catch(err => console.error("Error /toogle_led.json:", err));
}

// ======================
// ⏰ Hora desde /time.json
// ======================
function updateTime() {
    fetch('/time.json', { cache: 'no-store' })
        .then(resp => {
            if (!resp.ok) throw new Error("HTTP " + resp.status);
            return resp.json();
        })
        .then(data => {
            const h = String(data["hour"]).padStart(2, '0');
            const m = String(data["min"]).padStart(2, '0');
            const s = String(data["sec"]).padStart(2, '0');

            const span = document.getElementById("esp32_time");
            if (span) {
                span.textContent = `${h}:${m}:${s}`;
            }
        })
        .catch(err => {
            console.error("Error leyendo /time.json:", err);
        });
}
