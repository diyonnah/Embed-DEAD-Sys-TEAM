# S4-with-UI.ino — Function Explanations

This file serves as a detailed reference for every function used in `S4-with-UI.ino`. The sketch runs on an **ESP32 WROOM** microcontroller, hosts a Wi-Fi web server, and lets a browser control three LEDs using three different interaction modes: basic buttons, toggle switches, and a PWM brightness slider.

---

## Table of Contents

1. [Libraries & Global Variables](#1-libraries--global-variables)
2. [Arduino Core Functions](#2-arduino-core-functions)
   - [setup()](#setupvoid)
   - [loop()](#loopvoid)
3. [ESP32 / Arduino Built-in Functions](#3-esp32--arduino-built-in-functions)
4. [Web Server Handler Functions (C++)](#4-web-server-handler-functions-c)
   - [handleRoot()](#handlerootvoid)
   - [handleLED1on / handleLED1off](#handleled1on--handleled1off)
   - [handleLED2on / handleLED2off](#handleled2on--handleled2off)
   - [handleLED3on / handleLED3off](#handleled3on--handleled3off)
   - [handleLED1toggle / handleLED2toggle / handleLED3toggle](#handleled1toggle--handleled2toggle--handleled3toggle)
   - [handleLED3brightness()](#handleled3brightnessvoid)
   - [handleStatus()](#handlestatusvoid)
5. [JavaScript UI Functions (Client-Side)](#5-javascript-ui-functions-client-side)
   - [openTab()](#opentabtabname)
   - [setBadge()](#setbadgeid-state)
   - [setDots()](#setdotsled-state)
   - [updateUI()](#updateuiled-state-brightness)
   - [controlLED()](#controlledled-action)
   - [toggleLED()](#toggleledled)
   - [updateKnob()](#updateknobval)
   - [sendBrightness()](#sendbrrightnessval)
   - [setInterval() — Polling](#setinterval--polling)

---

## 1. Libraries & Global Variables

```cpp
#include <WiFi.h>
#include <WebServer.h>
```

| Item | Purpose |
|------|---------|
| `WiFi.h` | Gives the ESP32 the ability to connect to a Wi-Fi network, so it can communicate over the internet or a local area network. Without this, the board cannot reach a router. |
| `WebServer.h` | Provides a ready-made HTTP server. Instead of writing raw TCP socket code, this library lets you register URL routes and send responses with a few lines of code. |

```cpp
const char* ssid     = "CPE WIFI";
const char* password = "CP3Wi-Fi2025**";
WebServer server(80);
```

`server(80)` — Port **80** is the default HTTP port. Any browser that visits `http://<IP>` automatically uses port 80, so no special port number is needed in the URL.

```cpp
const int ledPin1 = 16; // Blue
const int ledPin2 = 18; // Red
const int ledPin3 = 27; // Yellow

bool led1State = false;
bool led2State = false;
bool led3State = false;

int  led3Brightness = 0;
const int pwmFreq       = 5000;
const int pwmResolution = 8;   // 0–255
```

- `ledPinX` constants map human-readable names to the physical GPIO pin numbers on the board, reducing the chance of typos.
- `ledXState` booleans track the current on/off state of each LED **in memory**, so the web page can reflect the correct state even after it is refreshed.
- `led3Brightness` tracks the PWM duty cycle (0 = fully off, 255 = fully on).
- `pwmFreq = 5000` Hz is fast enough that the human eye cannot see the LED flickering, yet slow enough for the ESP32's PWM hardware.
- `pwmResolution = 8` gives 256 discrete brightness steps (2⁸ = 256), which is plenty of granularity for visible LED dimming.

---

## 2. Arduino Core Functions

### `setup(void)`

```cpp
void setup() { ... }
```

**What it does:** Runs exactly **once** when the ESP32 powers on or resets. Everything needed to initialise hardware and network connections goes here.

**Why it exists:** Arduino's execution model splits initialisation (one-time) and the main program loop (repeated). `setup()` is guaranteed to finish before `loop()` starts, so you can rely on everything being ready when the main logic begins.

**Key actions inside `setup()`:**

| Call | Reason |
|------|--------|
| `Serial.begin(115200)` | Opens the USB serial port at 115 200 baud so you can read debug messages in the Arduino Serial Monitor. |
| `pinMode(ledPin1/2, OUTPUT)` | Configures GPIO 16 and 18 as output pins so the ESP32 can drive current to the LEDs. |
| `digitalWrite(ledPin1/2, LOW)` | Ensures both LEDs start in the **OFF** state to match the initial `false` values of `led1State` / `led2State`. |
| `ledcAttach(ledPin3, pwmFreq, pwmResolution)` | Attaches PWM to GPIO 27 with the configured frequency and resolution. |
| `ledcWrite(ledPin3, 0)` | Sets the initial PWM duty cycle to 0 (LED off). |
| `WiFi.begin(ssid, password)` | Starts the Wi-Fi connection process. |
| `while (WiFi.status() != WL_CONNECTED)` | Blocks until the connection is established. The `delay(500)` inside prevents hammering the chip with constant status checks. |
| `server.on("/route", handler)` | Registers every URL route with its corresponding handler function. |
| `server.begin()` | Starts listening for incoming HTTP connections. |

---

### `loop(void)`

```cpp
void loop() {
  server.handleClient();
}
```

**What it does:** Runs **continuously** in an infinite loop after `setup()` finishes.

**Why it is just one line:** The ESP32's `WebServer` library is **non-blocking**. `server.handleClient()` checks whether any browser has sent an HTTP request. If one has, it calls the matching handler function, sends a response, and returns immediately. If there is nothing to handle, it returns right away too — keeping the loop extremely fast and responsive.

---

## 3. ESP32 / Arduino Built-in Functions

These are library or framework functions called throughout the sketch.

| Function | Where Used | What It Does |
|----------|-----------|--------------|
| `Serial.begin(baud)` | `setup()` | Initialises serial communication at the given baud rate. Required before any `Serial.print()` call. |
| `Serial.print(x)` / `Serial.println(x)` | `setup()` | Sends text to the serial monitor. `println` appends a newline; `print` does not. Used for debugging Wi-Fi connection status. |
| `Serial.flush()` | `setup()` | Waits until all bytes in the serial transmit buffer have been sent. Prevents data loss before a `delay()`. |
| `delay(ms)` | `setup()` | Pauses execution for the given number of milliseconds. Used inside the Wi-Fi connection loop to avoid checking too frequently. |
| `pinMode(pin, mode)` | `setup()` | Sets a GPIO pin as `INPUT` or `OUTPUT`. LED pins must be `OUTPUT` before `digitalWrite()` can control them. |
| `digitalWrite(pin, value)` | handlers | Sets a digital output pin to `HIGH` (3.3 V, LED on) or `LOW` (0 V, LED off). Used for LEDs 1 and 2. |
| `ledcAttach(pin, freq, resolution)` | `setup()` | Attaches a PWM signal to a GPIO pin on the ESP32. The modern API (ESP-IDF v5+) replaces the older `ledcSetup` + `ledcAttachPin` pair. |
| `ledcWrite(pin, dutyCycle)` | handlers | Sets the PWM duty cycle on a previously attached pin. A duty cycle of 0 means the LED is off; 255 (for 8-bit resolution) means fully on. |
| `WiFi.begin(ssid, pass)` | `setup()` | Instructs the Wi-Fi chip to connect to the specified network. |
| `WiFi.status()` | `setup()` | Returns the current Wi-Fi connection status. `WL_CONNECTED` means the connection succeeded. |
| `WiFi.localIP()` | `setup()` | Returns the IP address assigned by the router, which is printed to serial so you know what address to type in the browser. |
| `constrain(val, min, max)` | `handleLED3brightness()` | Clamps a value to ensure it stays within `[min, max]`. Prevents out-of-range PWM values if a manually crafted HTTP request sends a bad number. |
| `server.on(route, handler)` | `setup()` | Registers a URL path with a C++ function. When a browser visits that path, the handler function is called automatically. |
| `server.begin()` | `setup()` | Starts the web server so it can accept incoming connections. |
| `server.handleClient()` | `loop()` | Processes any pending HTTP requests. Must be called regularly (every loop iteration) to keep the server responsive. |
| `server.send(code, type, body)` | All handlers | Sends an HTTP response to the browser. `code` is the HTTP status (200 = OK), `type` is the content type (e.g., `"text/html"`), and `body` is the content. |
| `server.hasArg(name)` | `handleLED3brightness()` | Returns `true` if the current HTTP request contains a query parameter with the given name (e.g., `?value=128`). |
| `server.arg(name)` | `handleLED3brightness()` | Returns the value of a named query parameter as a `String`. |
| `.toInt()` | `handleLED3brightness()` | Converts an Arduino `String` to an `int`. Used to parse the brightness number from the URL query string. |

---

## 4. Web Server Handler Functions (C++)

Every handler below follows the same pattern:
1. Read any inputs (GPIO state or query arguments).
2. Perform the hardware action (digitalWrite / ledcWrite).
3. Update the in-memory state variable.
4. Send an HTTP response back to the browser.

---

### `handleRoot(void)`

```cpp
void handleRoot() { ... }
```

**What it does:** Builds the **entire HTML/CSS/JS web page** as a C++ string and sends it to the browser in one HTTP response.

**Why it is done this way:** The ESP32 has no file system by default, so there are no `.html` files to serve. Instead, the page is generated dynamically in C++ strings. This means the page can embed **live state** (e.g., `led1State`) directly into the HTML so the initial render shows the correct on/off position without a separate status request.

**Structure of the generated page:**
- `<meta name='viewport' ...>` — Makes the page scale correctly on mobile screens.
- CSS variables (`:root { --bg: ... }`) — Centralised design tokens so colours only need changing in one place.
- `backdrop-filter: blur(...)` — Frosted-glass card effect on modern browsers.
- Three tabs (Basic, Toggle, PWM) — Separate the functionality of S1, S2, and S3 in one unified UI.
- Embedded `<script>` — AJAX fetch calls so the page updates **without full reloads**.

---

### `handleLED1on` / `handleLED1off`

```cpp
void handleLED1on()  { digitalWrite(ledPin1, HIGH); led1State = true;  server.send(200, "text/plain", "1"); }
void handleLED1off() { digitalWrite(ledPin1, LOW);  led1State = false; server.send(200, "text/plain", "0"); }
```

**What they do:** Directly turn LED 1 (Blue, GPIO 16) on or off.

**Why `"1"` / `"0"` is returned:** The browser JavaScript reads this plain-text response and converts it to a boolean (`state === '1'`) to decide whether to show the ON or OFF badge and dot without reloading the page.

The same logic applies identically to `handleLED2on` / `handleLED2off` for LED 2 (Red, GPIO 18).

---

### `handleLED2on` / `handleLED2off`

Same pattern as LED 1 but targeting `ledPin2` (GPIO 18) and `led2State`.

---

### `handleLED3on` / `handleLED3off`

```cpp
void handleLED3on() {
  ledcWrite(ledPin3, 255);
  led3State = true;
  led3Brightness = 255;
  server.send(200, "text/plain", "1");
}
```

**Why `ledcWrite` instead of `digitalWrite`?** LED 3 is on a PWM channel. Once a pin is attached to PWM via `ledcAttach()`, you must use `ledcWrite()` to control it. `digitalWrite()` would conflict with the PWM hardware.

Setting `led3Brightness = 255` keeps the brightness variable in sync so the slider in Tab 3 reflects the correct value during the next status poll.

---

### `handleLED1toggle` / `handleLED2toggle` / `handleLED3toggle`

```cpp
void handleLED1toggle() {
  led1State = !led1State;
  digitalWrite(ledPin1, led1State ? HIGH : LOW);
  server.send(200, "text/plain", led1State ? "1" : "0");
}
```

**What it does:** Flips the current state of the LED. If it was on, it turns off; if it was off, it turns on.

**Why a separate toggle route?** The Toggle tab uses a checkbox switch widget. When the user clicks the switch, JavaScript does not know the current server state — it just knows the user toggled something. A single `/ledXtoggle` endpoint handles this cleanly without needing to track state on the client side.

**Why `!led1State`?** The `!` (logical NOT) operator in C++ flips a boolean — `true` becomes `false` and vice versa. This is the most concise way to toggle.

---

### `handleLED3brightness(void)`

```cpp
void handleLED3brightness() {
  if (server.hasArg("value")) {
    led3Brightness = server.arg("value").toInt();
    led3Brightness = constrain(led3Brightness, 0, 255);
    ledcWrite(ledPin3, led3Brightness);
    led3State = (led3Brightness > 0);
  }
  server.send(200, "text/plain", "OK");
}
```

**What it does:** Reads a brightness value from the URL query string (e.g., `/led3brightness?value=128`) and applies it as a PWM duty cycle to LED 3.

**Why `constrain()`?** Never trust user input. A manually crafted URL could pass a value like `500` or `-10`, which would cause `ledcWrite()` to receive an out-of-range value. `constrain()` clamps it safely to `[0, 255]`.

**Why `led3State = (led3Brightness > 0)`?** This automatically keeps the on/off state consistent with the brightness. If brightness is 0, the LED is off; any positive value means it is on. This ensures the badges and dots in the UI stay correct without extra logic.

---

### `handleStatus(void)`

```cpp
void handleStatus() {
  String json = "{";
  json += "\"led1\":" + String(led1State ? "true" : "false") + ",";
  json += "\"led2\":" + String(led2State ? "true" : "false") + ",";
  json += "\"led3\":" + String(led3State ? "true" : "false") + ",";
  json += "\"bright3\":" + String(led3Brightness);
  json += "}";
  server.send(200, "application/json", json);
}
```

**What it does:** Returns the current state of all LEDs and the brightness level as a **JSON object**.

**Why JSON?** JSON is the standard data format for AJAX communication. The browser's `fetch()` call can parse it with `.json()` and read individual fields by name (`data.led1`, `data.bright3`), which is clear and easy to maintain.

**Why is this needed?** If two people open the web page at the same time, one person's actions would not be visible to the other without polling. The JavaScript calls this endpoint every 2 seconds to keep all open browser tabs in sync.

---

## 5. JavaScript UI Functions (Client-Side)

These functions run **in the browser**, not on the ESP32. They update the page without reloading it.

---

### `openTab(tabName)`

```javascript
function openTab(tabName) { ... }
```

**What it does:** Hides all tab panels, removes the `active` class from all tab buttons, then shows only the selected tab panel and marks its button as active.

**Why it works this way:** All three tab content `<div>`s exist in the DOM at once — only their `display` CSS property changes. This avoids re-fetching the page from the server when switching tabs.

---

### `setBadge(id, state)`

```javascript
function setBadge(id, state) {
  const el = document.getElementById(id);
  el.innerText = state ? 'ON' : 'OFF';
  el.className = 'badge ' + (state ? 'badge-on' : 'badge-off');
}
```

**What it does:** Finds a badge element by its ID, changes its text to `ON` or `OFF`, and swaps its CSS class to control the colour (green for ON, grey for OFF).

**Why a reusable function?** Each LED has a badge in multiple tabs. Rather than copy-pasting the same two lines six times, one function handles them all. If the styling ever changes, only one place needs to be updated.

---

### `setDots(led, state)`

```javascript
function setDots(led, state) {
  document.querySelectorAll('[data-led="' + led + '"]').forEach(dot => {
    dot.classList.toggle('lit', state);
  });
}
```

**What it does:** Finds every LED indicator dot `<span>` across all tabs that belongs to a given LED number (via the `data-led` attribute) and adds or removes the `lit` CSS class, which applies the glow effect.

**Why `querySelectorAll`?** Because the same LED dot appears in multiple tabs. A single `data-led` attribute selector updates them all at once instead of hard-coding individual IDs.

**Why `classList.toggle(class, force)`?** The second argument is a boolean that forces the class to be added (`true`) or removed (`false`), making it more predictable than calling `add` / `remove` conditionally.

---

### `updateUI(led, state, brightness)`

```javascript
function updateUI(led, state, brightness = null) { ... }
```

**What it does:** The **master UI sync function**. Given an LED number and its new state (and optionally a brightness value), it calls `setDots()`, `setBadge()` for every relevant badge, updates the checkbox in the Toggle tab, and synchronises the PWM slider and number display.

**Why centralise all UI updates here?** Every path that changes an LED state — button clicks, toggles, slider changes, and the polling loop — all call `updateUI()`. This single source of truth prevents the UI from getting out of sync across tabs.

---

### `controlLED(led, action)`

```javascript
function controlLED(led, action) {
  fetch('/led' + led + action)
    .then(response => response.text())
    .then(state => updateUI(led, state === '1'));
}
```

**What it does:** Sends an HTTP GET request to a URL like `/led1on` or `/led2off`, waits for the plain-text response (`"1"` or `"0"`), then updates the UI.

**Why `fetch()`?** `fetch()` is the modern browser API for making asynchronous HTTP requests. It returns a **Promise**, so `.then()` chains run only after the server responds — this prevents the UI from updating before the ESP32 confirms the action.

**Why compare `state === '1'`?** The server sends the plain string `"1"` or `"0"`. Comparing strictly to `'1'` converts it to the boolean the `updateUI()` function expects.

---

### `toggleLED(led)`

```javascript
function toggleLED(led) {
  fetch('/led' + led + 'toggle')
    .then(response => response.text())
    .then(state => updateUI(led, state === '1'));
}
```

**What it does:** Sends a toggle request to the server (e.g., `/led1toggle`) and updates the UI with the returned new state.

**Why not just flip the checkbox state locally?** If two browsers are open, local state would diverge. By always reading the **server's** returned state, the UI stays consistent regardless of how many clients are connected.

---

### `updateKnob(val)`

```javascript
function updateKnob(val) {
  document.getElementById('brightVal').innerText = val;
  const pct = Math.round(val / 255 * 100);
  document.getElementById('brightSlider').style.background =
    'linear-gradient(to right, #f59e0b ' + pct + '%, rgba(...) ' + pct + '%)';
}
```

**What it does:** Updates the large brightness number display and applies a gradient fill to the range slider track so the filled portion grows as brightness increases.

**Why update the gradient manually?** CSS `<input type="range">` does not natively colour the filled track. Applying a gradient via JavaScript simulates a "fill" effect that gives the user clear visual feedback about the selected level.

**Why called on `oninput`?** `oninput` fires as the thumb is dragged, giving real-time visual feedback. `onchange` only fires when the thumb is released (used to actually send the request to the server, avoiding a flood of network requests while dragging).

---

### `sendBrightness(val)`

```javascript
function sendBrightness(val) {
  fetch('/led3brightness?value=' + val)
    .then(() => updateUI(3, val > 0, val));
}
```

**What it does:** Sends the chosen brightness value to the server via a URL query parameter (e.g., `/led3brightness?value=128`) and updates the UI optimistically once the request is sent.

**Why separate from `updateKnob()`?** `updateKnob()` is purely visual and runs on every pixel of slider movement. `sendBrightness()` triggers an actual network request and is only called `onchange` (when the user releases the slider), avoiding sending hundreds of requests per drag.

**Why `val > 0` for the state?** A brightness greater than zero means the LED is on. This keeps the badge and dot consistent without needing the server to send back a state value.

---

### `setInterval()` — Polling

```javascript
setInterval(() => {
  fetch('/status')
    .then(res => res.json())
    .then(data => {
      updateUI(1, data.led1);
      updateUI(2, data.led2);
      updateUI(3, data.led3, data.bright3);
    });
}, 2000);
```

**What it does:** Every **2 seconds**, fetches `/status` from the ESP32, parses the JSON response, and calls `updateUI()` for all three LEDs.

**Why polling instead of WebSockets?** WebSockets would be more efficient for real-time sync, but they require more complex server code. For a simple embedded project with infrequent state changes, polling every 2 seconds is straightforward and sufficient.

**Why 2 seconds?** Short enough that a second user's actions appear quickly on your screen; long enough not to overwhelm the ESP32 with requests. The ESP32 is a microcontroller handling a simple task, so keeping the polling interval reasonable prevents performance issues.

---

## Summary Table

| Function | Location | Type | Role |
|----------|---------|------|------|
| `setup()` | ESP32 | Arduino core | One-time hardware & network init |
| `loop()` | ESP32 | Arduino core | Keeps web server responsive |
| `handleRoot()` | ESP32 | HTTP handler | Serves the full web page |
| `handleLED1on/off` | ESP32 | HTTP handler | Directly turns LED 1 on/off |
| `handleLED2on/off` | ESP32 | HTTP handler | Directly turns LED 2 on/off |
| `handleLED3on/off` | ESP32 | HTTP handler | PWM LED 3 full on/full off |
| `handleLED1toggle` | ESP32 | HTTP handler | Flips LED 1 state |
| `handleLED2toggle` | ESP32 | HTTP handler | Flips LED 2 state |
| `handleLED3toggle` | ESP32 | HTTP handler | Flips LED 3 via PWM |
| `handleLED3brightness()` | ESP32 | HTTP handler | Sets PWM duty cycle for LED 3 |
| `handleStatus()` | ESP32 | HTTP handler | Returns JSON state for polling |
| `openTab()` | Browser JS | UI | Switches visible tab |
| `setBadge()` | Browser JS | UI | Updates ON/OFF label & colour |
| `setDots()` | Browser JS | UI | Toggles glowing LED dot |
| `updateUI()` | Browser JS | UI | Master sync across all tabs |
| `controlLED()` | Browser JS | AJAX | Sends on/off request |
| `toggleLED()` | Browser JS | AJAX | Sends toggle request |
| `updateKnob()` | Browser JS | UI | Real-time slider visual update |
| `sendBrightness()` | Browser JS | AJAX | Sends brightness to server |
| `setInterval` (polling) | Browser JS | AJAX | Syncs all clients every 2 s |
