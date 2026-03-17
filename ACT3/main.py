import network
import socket
import dht
import math
import json
import time
from machine import Pin, I2C

# =========================
# Configuration
# =========================
WIFI_SSID = "happy new year"
WIFI_PASSWORD = "20262026"

AP_SSID = "ESP32-ML-FAN"
AP_PASSWORD = "12345678"

DHT_PIN = 13
BUTTON_INC_PIN = 34
BUTTON_DEC_PIN = 35

FAN_LED_PIN = 23
LEVEL_LED1_PIN = 2
LEVEL_LED2_PIN = 16
LEVEL_LED3_PIN = 5
LEVEL_LED4_PIN = 19

LCD_SCL_PIN = 22
LCD_SDA_PIN = 21
LCD_ADDR = 0x27
LCD_ROWS = 2
LCD_COLS = 16

READ_INTERVAL_MS = 2000
LCD_PAGE_INTERVAL_MS = 2000
DEBOUNCE_MS = 180

MIN_LEVEL = 1
MAX_LEVEL = 4

# Logistic regression parameters (from trained model)
# You can replace these with your own trained values from train_model.py
W_TEMP = -8.420
W_HUM = -0.709
BIAS = 306.606

# =========================
# Hardware setup
# =========================
sensor = dht.DHT11(Pin(DHT_PIN))
btn_inc = Pin(BUTTON_INC_PIN, Pin.IN, Pin.PULL_UP)
btn_dec = Pin(BUTTON_DEC_PIN, Pin.IN, Pin.PULL_UP)
fan_led = Pin(FAN_LED_PIN, Pin.OUT)
level_leds = [
    Pin(LEVEL_LED1_PIN, Pin.OUT),
    Pin(LEVEL_LED2_PIN, Pin.OUT),
    Pin(LEVEL_LED3_PIN, Pin.OUT),
    Pin(LEVEL_LED4_PIN, Pin.OUT),
]


class FallbackI2cLcd:
    LCD_CLR = 0x01
    LCD_HOME = 0x02
    LCD_ENTRY_MODE = 0x04
    LCD_ENTRY_INC = 0x02
    LCD_ON_CTRL = 0x08
    LCD_ON_DISPLAY = 0x04
    LCD_FUNCTION = 0x20
    LCD_FUNCTION_2LINES = 0x08
    LCD_DDRAM = 0x80

    MASK_RS = 0x01
    MASK_RW = 0x02
    MASK_EN = 0x04
    MASK_BACKLIGHT = 0x08

    def __init__(self, i2c, addr, rows, cols):
        self.i2c = i2c
        self.addr = addr
        self.rows = rows
        self.cols = cols
        self.backlight = self.MASK_BACKLIGHT
        self.row_offsets = [0x00, 0x40, 0x14, 0x54]

        time.sleep_ms(50)
        self._write_init_nibble(0x30)
        time.sleep_ms(5)
        self._write_init_nibble(0x30)
        time.sleep_ms(1)
        self._write_init_nibble(0x30)
        time.sleep_ms(1)
        self._write_init_nibble(0x20)
        time.sleep_ms(1)

        self._command(self.LCD_FUNCTION | self.LCD_FUNCTION_2LINES)
        self._command(self.LCD_ON_CTRL | self.LCD_ON_DISPLAY)
        self.clear()
        self._command(self.LCD_ENTRY_MODE | self.LCD_ENTRY_INC)

    def _expander_write(self, data):
        self.i2c.writeto(self.addr, bytes([data | self.backlight]))

    def _pulse(self, data):
        self._expander_write(data | self.MASK_EN)
        time.sleep_us(1)
        self._expander_write(data & ~self.MASK_EN)
        time.sleep_us(120)

    def _write_init_nibble(self, value):
        data = (value & 0xF0)
        self._expander_write(data)
        self._pulse(data)

    def _write_byte(self, value, mode=0):
        high = (value & 0xF0) | mode
        low = ((value << 4) & 0xF0) | mode
        self._expander_write(high)
        self._pulse(high)
        self._expander_write(low)
        self._pulse(low)

    def _command(self, cmd):
        self._write_byte(cmd, 0)
        if cmd == self.LCD_CLR or cmd == self.LCD_HOME:
            time.sleep_ms(2)

    def _data(self, value):
        self._write_byte(value, self.MASK_RS)

    def clear(self):
        self._command(self.LCD_CLR)
        time.sleep_ms(2)

    def move_to(self, col, row):
        row = max(0, min(self.rows - 1, row))
        col = max(0, min(self.cols - 1, col))
        self._command(self.LCD_DDRAM | (col + self.row_offsets[row]))

    def putstr(self, text):
        for ch in text:
            self._data(ord(ch))

lcd = None
last_lcd_line1 = ""
last_lcd_line2 = ""
last_lcd_refresh_ms = 0
last_lcd_retry_ms = 0


def init_lcd():
    try:
        i2c = I2C(0, scl=Pin(LCD_SCL_PIN), sda=Pin(LCD_SDA_PIN), freq=100000)
        scanned = i2c.scan()
        print("I2C scan:", scanned)
        if not scanned:
            return None

        lcd_addr = LCD_ADDR
        if lcd_addr not in scanned and 0x3F in scanned:
            lcd_addr = 0x3F
        elif lcd_addr not in scanned:
            lcd_addr = scanned[0]

        try:
            from i2c_lcd import I2cLcd
            lcd_obj = I2cLcd(i2c, lcd_addr, LCD_ROWS, LCD_COLS)
            lcd_obj.clear()
            lcd_obj.move_to(0, 0)
            lcd_obj.putstr("ML Fan Monitor")
            lcd_obj.move_to(0, 1)
            lcd_obj.putstr("LCD Connected")
            print("LCD ready via i2c_lcd @", hex(lcd_addr))
            return lcd_obj
        except Exception as e:
            print("i2c_lcd init failed, fallback:", e)

        lcd_obj = FallbackI2cLcd(i2c, lcd_addr, LCD_ROWS, LCD_COLS)
        lcd_obj.clear()
        lcd_obj.move_to(0, 0)
        lcd_obj.putstr("ML Fan Monitor")
        lcd_obj.move_to(0, 1)
        lcd_obj.putstr("Fallback LCD OK")
        print("LCD ready via fallback @", hex(lcd_addr))
        return lcd_obj

    except Exception as e:
        print("LCD not available:", e)
        return None


lcd = init_lcd()


# =========================
# Runtime state
# =========================
state = {
    "temperature": 0.0,
    "humidity": 0.0,
    "prediction": "N/A",
    "probability": 0.0,
    "fan_level": 1,
    "fan_on": False,
    "mode": "auto",  # auto | manual
    "wifi_mode": "STA",
    "ip": "0.0.0.0",
    "last_update_ms": 0,
}

button_state = {
    "inc_last": 1,
    "dec_last": 1,
    "inc_last_time": 0,
    "dec_last_time": 0,
}

lcd_page = 0
last_lcd_page_ms = 0


# =========================
# Helpers
# =========================
def sigmoid(x):
    return 1.0 / (1.0 + math.exp(-x))


def predict_environment(temp, hum):
    z = (W_TEMP * temp) + (W_HUM * hum) + BIAS
    prob = sigmoid(z)
    label = "Hot/Humid" if prob > 0.5 else "Comfortable"
    return label, prob


def auto_fan_level(probability):
    if probability < 0.40:
        return 1
    if probability < 0.55:
        return 2
    if probability < 0.70:
        return 3
    return 4


def set_fan_level(level):
    level = max(MIN_LEVEL, min(MAX_LEVEL, level))
    state["fan_level"] = level
    state["fan_on"] = True
    fan_led.value(1)
    for i, led in enumerate(level_leds):
        led.value(1 if i < level else 0)


def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PASSWORD)

    print("Connecting to Wi-Fi", end="")
    start = time.ticks_ms()
    while not wlan.isconnected() and time.ticks_diff(time.ticks_ms(), start) < 15000:
        print(".", end="")
        time.sleep_ms(500)
    print()

    if wlan.isconnected():
        ip = wlan.ifconfig()[0]
        state["wifi_mode"] = "STA"
        state["ip"] = ip
        print("Connected:", wlan.ifconfig())
        return wlan

    print("Wi-Fi failed. Switching to AP mode.")
    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid=AP_SSID, password=AP_PASSWORD)

    while not ap.active():
        time.sleep_ms(100)

    ip = ap.ifconfig()[0]
    state["wifi_mode"] = "AP"
    state["ip"] = ip
    print("AP Started:", ap.ifconfig())
    return ap


def read_sensor_and_predict():
    try:
        sensor.measure()
        temp = sensor.temperature()
        hum = sensor.humidity()

        label, prob = predict_environment(temp, hum)

        state["temperature"] = temp
        state["humidity"] = hum
        state["prediction"] = label
        state["probability"] = prob
        state["last_update_ms"] = time.ticks_ms()

        if state["mode"] == "auto":
            set_fan_level(auto_fan_level(prob))

    except Exception as e:
        print("Sensor read error:", e)


def process_buttons():
    now = time.ticks_ms()

    inc_now = btn_inc.value()
    dec_now = btn_dec.value()

    # Active LOW buttons
    if inc_now == 0 and button_state["inc_last"] == 1:
        if time.ticks_diff(now, button_state["inc_last_time"]) > DEBOUNCE_MS:
            button_state["inc_last_time"] = now
            state["mode"] = "manual"
            set_fan_level(state["fan_level"] + 1)

    if dec_now == 0 and button_state["dec_last"] == 1:
        if time.ticks_diff(now, button_state["dec_last_time"]) > DEBOUNCE_MS:
            button_state["dec_last_time"] = now
            state["mode"] = "manual"
            set_fan_level(state["fan_level"] - 1)

    button_state["inc_last"] = inc_now
    button_state["dec_last"] = dec_now


def lcd_pad(text, width):
    text = str(text)
    if len(text) >= width:
        return text[:width]
    return text + (" " * (width - len(text)))


def lcd_write(line1, line2):
    global lcd
    global last_lcd_line1, last_lcd_line2, last_lcd_refresh_ms
    global LCD_REFRESH_MS, LCD_COLS

    if "last_lcd_line1" not in globals():
        last_lcd_line1 = ""
    if "last_lcd_line2" not in globals():
        last_lcd_line2 = ""
    if "last_lcd_refresh_ms" not in globals():
        last_lcd_refresh_ms = 0
    if "LCD_REFRESH_MS" not in globals():
        LCD_REFRESH_MS = 600
    if "LCD_COLS" not in globals():
        LCD_COLS = 16

    if not lcd:
        return
    try:
        now = time.ticks_ms()
        if (
            line1 == last_lcd_line1
            and line2 == last_lcd_line2
            and time.ticks_diff(now, last_lcd_refresh_ms) < LCD_REFRESH_MS
        ):
            return

        line1_padded = lcd_pad(line1, LCD_COLS)
        line2_padded = lcd_pad(line2, LCD_COLS)

        lcd.move_to(0, 0)
        lcd.putstr(line1_padded)
        lcd.move_to(0, 1)
        lcd.putstr(line2_padded)

        last_lcd_line1 = line1
        last_lcd_line2 = line2
        last_lcd_refresh_ms = now
    except Exception as e:
        print("LCD update error:", e)
        lcd = None


def update_lcd():
    global lcd, lcd_page, last_lcd_page_ms, last_lcd_retry_ms
    global LCD_RETRY_MS, LCD_PAGE_INTERVAL_MS

    if "last_lcd_retry_ms" not in globals():
        last_lcd_retry_ms = 0
    if "lcd_page" not in globals():
        lcd_page = 0
    if "last_lcd_page_ms" not in globals():
        last_lcd_page_ms = 0
    if "LCD_RETRY_MS" not in globals():
        LCD_RETRY_MS = 5000
    if "LCD_PAGE_INTERVAL_MS" not in globals():
        LCD_PAGE_INTERVAL_MS = 2000

    now = time.ticks_ms()
    if not lcd:
        if time.ticks_diff(now, last_lcd_retry_ms) >= LCD_RETRY_MS:
            last_lcd_retry_ms = now
            lcd = init_lcd()
        return

    if time.ticks_diff(now, last_lcd_page_ms) > LCD_PAGE_INTERVAL_MS:
        lcd_page = (lcd_page + 1) % 2
        last_lcd_page_ms = now

    temp = state["temperature"]
    hum = state["humidity"]
    prediction = state["prediction"]
    level = state["fan_level"]
    mode = state["mode"].upper()

    if lcd_page == 0:
        lcd_write("T:{:>2}C H:{:>2}%".format(int(temp), int(hum)), "{} L{}".format(mode, level))
    else:
        short_pred = "Comfort" if prediction == "Comfortable" else "Hot/Humid"
        lcd_write("Prediction:", "{} L{}".format(short_pred[:9], level))


def build_status_payload():
    return {
        "temperature": state["temperature"],
        "humidity": state["humidity"],
        "prediction": state["prediction"],
        "probability": round(state["probability"], 3),
        "fan_level": state["fan_level"],
        "fan_on": state["fan_on"],
        "mode": state["mode"],
        "wifi_mode": state["wifi_mode"],
        "ip": state["ip"],
    }


def send_response(client, status_code, content_type, body):
    reason = "OK" if status_code == 200 else "Bad Request"
    if status_code == 404:
        reason = "Not Found"

    client.send("HTTP/1.1 {} {}\r\n".format(status_code, reason))
    client.send("Content-Type: {}\r\n".format(content_type))
    client.send("Connection: close\r\n\r\n")

    if isinstance(body, str):
        client.send(body)
    else:
        client.sendall(body)


def parse_path(request_line):
    try:
        parts = request_line.split(" ")
        return parts[1]
    except Exception:
        return "/"


def parse_query(path):
    if "?" not in path:
        return path, {}

    route, query_str = path.split("?", 1)
    args = {}
    for pair in query_str.split("&"):
        if "=" in pair:
            k, v = pair.split("=", 1)
            args[k] = v
    return route, args


HTML_PAGE = """<!DOCTYPE html>
<html>
<head>
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />
  <title>ACT3 Smart Fan Monitor</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background: #0f172a; color: #e2e8f0; }
    .card { background: #1e293b; border-radius: 12px; padding: 16px; max-width: 420px; margin: auto; }
    .row { display: flex; justify-content: space-between; margin: 8px 0; }
    .controls { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 14px; }
    button { border: 0; border-radius: 8px; padding: 10px; font-weight: bold; cursor: pointer; }
    .primary { background: #2563eb; color: white; }
    .warn { background: #f59e0b; color: #111827; }
    .ok { background: #22c55e; color: #052e16; }
  </style>
</head>
<body>
  <div class=\"card\">
    <h2>ACT3 Smart Fan (ML)</h2>
    <div class=\"row\"><span>Temperature</span><strong id=\"temp\">--</strong></div>
    <div class=\"row\"><span>Humidity</span><strong id=\"hum\">--</strong></div>
    <div class=\"row\"><span>Prediction</span><strong id=\"pred\">--</strong></div>
    <div class=\"row\"><span>Fan Level</span><strong id=\"level\">--</strong></div>
    <div class=\"row\"><span>Mode</span><strong id=\"mode\">--</strong></div>
    <div class=\"row\"><span>IP</span><strong id=\"ip\">--</strong></div>

    <div class=\"controls\">
      <button class=\"primary\" onclick=\"setMode('auto')\">AUTO</button>
      <button class=\"warn\" onclick=\"setMode('manual')\">MANUAL</button>
      <button class=\"ok\" onclick=\"setFan(-1)\">LEVEL -</button>
      <button class=\"ok\" onclick=\"setFan(1)\">LEVEL +</button>
    </div>
  </div>

<script>
let levelCache = 1;

async function loadStatus() {
  const res = await fetch('/api/status');
  const s = await res.json();

  document.getElementById('temp').textContent = s.temperature + ' C';
  document.getElementById('hum').textContent = s.humidity + ' %';
  document.getElementById('pred').textContent = s.prediction;
  document.getElementById('level').textContent = s.fan_level;
  document.getElementById('mode').textContent = s.mode.toUpperCase();
  document.getElementById('ip').textContent = s.ip + ' (' + s.wifi_mode + ')';

  levelCache = s.fan_level;
}

async function setMode(mode) {
  await fetch('/api/mode?value=' + mode);
  await loadStatus();
}

async function setFan(delta) {
  let next = levelCache + delta;
  if (next < 1) next = 1;
  if (next > 4) next = 4;
  await fetch('/api/fan?level=' + next);
  await loadStatus();
}

setInterval(loadStatus, 2000);
loadStatus();
</script>
</body>
</html>
"""


def handle_request(client, path):
    route, args = parse_query(path)

    if route == "/":
        send_response(client, 200, "text/html", HTML_PAGE)
        return

    if route == "/api/status":
        send_response(client, 200, "application/json", json.dumps(build_status_payload()))
        return

    if route == "/api/mode":
        mode = args.get("value", "").lower()
        if mode in ("auto", "manual"):
            state["mode"] = mode
            if mode == "auto":
                set_fan_level(auto_fan_level(state["probability"]))
        send_response(client, 200, "application/json", json.dumps(build_status_payload()))
        return

    if route == "/api/fan":
        try:
            req_level = int(args.get("level", state["fan_level"]))
            state["mode"] = "manual"
            set_fan_level(req_level)
        except Exception:
            pass
        send_response(client, 200, "application/json", json.dumps(build_status_payload()))
        return

    send_response(client, 404, "text/plain", "Not found")


# =========================
# Main
# =========================
wlan = connect_wifi()
if "APP_VERSION" not in globals():
    APP_VERSION = "ACT3_LCD_FIX_v2"
print("Running", APP_VERSION)
set_fan_level(1)
read_sensor_and_predict()
update_lcd()

addr = socket.getaddrinfo("0.0.0.0", 80)[0][-1]
server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(addr)
server.listen(1)
server.settimeout(0.2)

print("Web server running at http://{}".format(state["ip"]))

last_read = 0

while True:
    now = time.ticks_ms()

    if time.ticks_diff(now, last_read) > READ_INTERVAL_MS:
        last_read = now
        read_sensor_and_predict()

    process_buttons()
    update_lcd()

    try:
        client, client_addr = server.accept()
        client.settimeout(1.0)

        req = client.recv(1024)
        request_line = req.decode().split("\r\n")[0]
        path = parse_path(request_line)

        handle_request(client, path)
        client.close()

    except OSError:
        # timeout waiting for a client; continue loop
        pass
    except Exception as e:
        print("Request error:", e)
        try:
            client.close()
        except Exception:
            pass

