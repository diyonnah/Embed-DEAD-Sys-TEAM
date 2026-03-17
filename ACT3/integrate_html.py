import network
import socket
import dht
from machine import Pin
import math
import json
import time

# --- WiFi Setup ---
ssid = "happy new year"
password = "20262026"

wifi = network.WLAN(network.STA_IF)
wifi.active(True)
wifi.connect(ssid, password)

while not wifi.isconnected():
    pass

print("Connected:", wifi.ifconfig())

# --- Sensor Setup ---
sensor = dht.DHT11(Pin(13))

w_temp = 0.32
w_hum = 0.18
bias = -12.7

def sigmoid(x):
    return 1 / (1 + math.exp(-x))

def predict(temp, hum):
    z = (w_temp * temp) + (w_hum * hum) + bias
    probability = sigmoid(z)
    return probability > 0.5  # True = Hot/Humid, False = Comfortable

# --- Socket Setup ---
addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]
server = socket.socket()
server.bind(addr)
server.listen(1)
print("Web server running on http://{}:{}".format(addr[0], addr[1]))

# --- HTML Template ---
html = """<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>DHT11 Temperature Tracker</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.2/dist/chart.umd.min.js"></script>
<style>
/* Include all your CSS from the provided HTML here */
body { background: #04080f; color: #f1f5f9; font-family: 'Inter', sans-serif; text-align:center; }
.metric-value { font-size:58px; font-weight:800; }
.temp-val { color:#06b6d4; }
.hum-val { color:#3b82f6; }
</style>
</head>
<body>
<h1>DHT11 Sensor Data</h1>
<div>
    <div>Temperature: <span id="tempVal">--</span> °C</div>
    <div>Humidity: <span id="humVal">--</span> %</div>
    <div>Status: <span id="fanStatus">Waiting...</span></div>
</div>
<script>
function fetchData() {
    fetch('/status')
    .then(resp => resp.json())
    .then(data => {
        document.getElementById('tempVal').innerText = data.temperature.toFixed(1);
        document.getElementById('humVal').innerText = data.humidity.toFixed(1);
        document.getElementById('fanStatus').innerText = data.fan ? 'Hot/Humid' : 'Comfortable';
    })
    .catch(err => console.log(err));
}
setInterval(fetchData, 2000);
fetchData();
</script>
</body>
</html>
"""

# --- Main Loop ---
while True:
    try:
        cl, addr = server.accept()
        print("Client connected from", addr)
        cl_file = cl.makefile('rwb', 0)
        request_line = cl_file.readline()
        # Read and discard headers
        while True:
            line = cl_file.readline()
            if not line or line == b'\r\n':
                break

        request = str(request_line)
        # Serve JSON for /status
        if '/status' in request:
            try:
                sensor.measure()
                temp = sensor.temperature()
                hum = sensor.humidity()
                fan_status = predict(temp, hum)
            except OSError:
                temp = 0
                hum = 0
                fan_status = False

            response = json.dumps({
                'temperature': temp,
                'humidity': hum,
                'fan': fan_status
            })
            cl.send('HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n')
            cl.send(response)
        else:
            # Serve HTML
            cl.send('HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n')
            cl.send(html)
        cl.close()
    except Exception as e:
        print("Error:", e)
