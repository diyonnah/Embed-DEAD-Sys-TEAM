import network
import socket
import dht
import machine
import time

# --- WiFi credentials ---
SSID = "happy new year"
PASSWORD = "20262026"

# --- DHT11 setup ---
sensor = dht.DHT11(machine.Pin(13))

# --- Connect to WiFi ---
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect(SSID, PASSWORD)

print("Connecting to WiFi...")
while not wlan.isconnected():
    time.sleep(0.5)
    print(".", end="")
print("\nConnected:", wlan.ifconfig()[0])

# --- HTML page (your full design) ---
INDEX_HTML = """<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DHT11 Temperature Tracker</title>
    ... (paste your full HTML here) ...
</head>
<body>
    ...
</body>
</html>"""

# --- Read sensor ---
def read_sensor():
    try:
        sensor.measure()
        return sensor.temperature(), sensor.humidity()
    except OSError:
        return None, None

# --- Build HTTP response ---
def send_response(client, body, content_type="text/html"):
    client.send("HTTP/1.1 200 OK\r\n")
    client.send("Content-Type: " + content_type + "\r\n")
    client.send("Connection: close\r\n\r\n")
    client.sendall(body)

# --- Start server ---
addr = socket.getaddrinfo("0.0.0.0", 80)[0][-1]
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(addr)
s.listen(5)
print("Web server running on http://" + wlan.ifconfig()[0])

fan_on = False
fan_level = 1

while True:
    try:
        client, addr = s.accept()
        request = client.recv(1024).decode()
        print("Request:", request.split("\r\n")[0])

        # Route: /status → return JSON for the live dashboard
        if "/status" in request:
            temp, hum = read_sensor()
            if temp is None:
                send_response(client, '{"error":"sensor error"}', "application/json")
            else:
                fan_on = temp >= 30
                import ujson
                data = ujson.dumps({
                    "temperature": temp,
                    "humidity": hum,
                    "fan": fan_on,
                    "level": fan_level
                })
                send_response(client, data, "application/json")

        # Route: /fan?intensity=N → update fan intensity
        elif "/fan" in request:
            try:
                intensity = int(request.split("intensity=")[1].split(" ")[0])
                fan_level = max(1, min(4, intensity))
            except:
                pass
            import ujson
            send_response(client, ujson.dumps({"level": fan_level}), "application/json")

        # Route: / → serve the full HTML dashboard
        else:
            send_response(client, INDEX_HTML)

        client.close()

    except Exception as e:
        print("Error:", e)
        try:
            client.close()
        except:
            pass