// These libraries let the ESP32 connect to WiFi and host a web server.
#include <WiFi.h>
#include <WebServer.h>

// Your WiFi network name and password so the ESP32 can connect to your router.
const char* ssid = "CPE WIFI";
const char* password = "CP3Wi-Fi2025**";

// Creates a web server that listens on port 80 (the standard HTTP port).
WebServer server(80);

// The GPIO pin numbers where each LED is physically connected on the ESP32.
const int ledPin1 = 16; // Blue
const int ledPin2 = 18; // Red
const int ledPin3 = 27; // Yellow

// These variables remember whether each LED is currently ON or OFF.
// They are used to show the correct status on the web page.
bool led1State = false;
bool led2State = false;
bool led3State = false;

// PWM (Pulse Width Modulation) settings for LED 3 (Yellow).
// PWM lets us control the brightness instead of just ON/OFF.
// Channel 0 is one of the 16 PWM channels available on the ESP32.
// 5000 Hz is the frequency (how fast it pulses). 8-bit resolution means
// brightness can be set from 0 (off) to 255 (full brightness).
int led3Brightness = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;  // 8-bit = 0 to 255

// This function builds and sends the entire web page to the browser whenever
// someone visits the ESP32's IP address. Everything inside is HTML/CSS/JS
// written as Arduino strings, because the ESP32 has no separate HTML file.
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  // Tells the browser to scale the page properly on mobile screens.
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 LED Control</title>";

  // =====================================================
  // CSS STYLES (Dito mag-eedit ang Web Designer mo)
  // All visual styling is here. Change colors, sizes, and
  // layout in this block without touching the C++ logic.
  // =====================================================
  html += "<style>";
  // General page style: centered text, grey background, no default margins.
  html += "body { font-family: Arial, sans-serif; text-align: center; background: #f0f0f0; margin: 0; padding: 0; }";
  html += "h2 { color: #333; margin-top: 20px; }";
  
  /* Tab bar at the top: each button takes equal space (flex: 1).
     The active tab gets a blue underline so the user knows which tab is open. */
  html += ".tab { display: flex; justify-content: center; background-color: #ddd; overflow: hidden; }";
  html += ".tab button { background-color: inherit; border: none; outline: none; cursor: pointer; padding: 14px 10px; font-size: 16px; flex: 1; transition: 0.3s; }";
  html += ".tab button:hover { background-color: #ccc; }";
  html += ".tab button.active { background-color: #fff; font-weight: bold; border-bottom: 3px solid #2196F3; }";
  // Tab content panels are hidden by default; JavaScript shows the active one.
  html += ".tabcontent { display: none; padding: 20px 10px; background: #fff; min-height: 400px; }";
  
  /* S1 - Large touch-friendly buttons. Blue for ON, red for OFF.
     width: 80% with max-width makes them resize nicely on any screen. */
  html += ".btn { padding: 15px 20px; margin: 10px; font-size: 16px; border-radius: 8px; border: none; cursor: pointer; background-color: #2196F3; color: white; width: 80%; max-width: 250px; }";
  html += ".btn-off { background-color: #f44336; }";
  
  /* S2 - Card layout for each LED with a toggle switch.
     The .switch/.slider combo is a pure-CSS checkbox styled to look like a toggle. */
  html += ".led-card { background: #fafafa; border-radius: 12px; padding: 20px; margin: 15px auto; width: 90%; max-width: 300px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }";
  html += ".switch { position: relative; display: inline-block; width: 60px; height: 34px; }";
  html += ".switch input { opacity: 0; width: 0; height: 0; }";
  html += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px; transition: .4s; }";
  html += ".slider:before { position: absolute; content: ''; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; border-radius: 50%; transition: .4s; }";
  html += "input:checked + .slider { background-color: #2196F3; }";
  html += "input:checked + .slider:before { transform: translateX(26px); }";
  html += ".status { font-size: 14px; margin-top: 8px; color: #555; }";
  
  /* S3 - Styling for the brightness slider (range input).
     The thumb (draggable circle) is enlarged to 28px for easier finger tapping. */
  html += ".knob-container { display: flex; flex-direction: column; align-items: center; margin: 10px 0; }";
  html += ".brightness-value { font-size: 24px; font-weight: bold; color: #FFA500; margin: 10px 0; }";
  html += "input[type=range].knob-range { -webkit-appearance: none; width: 100%; max-width: 250px; height: 8px; border-radius: 4px; background: #ddd; outline: none; margin: 15px 0; }";
  html += "input[type=range].knob-range::-webkit-slider-thumb { -webkit-appearance: none; width: 28px; height: 28px; border-radius: 50%; background: #FFA500; cursor: pointer; box-shadow: 0 2px 6px rgba(0,0,0,0.3); }";
  html += "</style></head><body>";

  html += "<h2>ESP32 LED Control</h2>";

  // =====================================================
  // TABS HEADER
  // Three buttons at the top. Clicking each one calls openTab()
  // in JavaScript to show the matching tab panel.
  // =====================================================
  html += "<div class='tab'>";
  html += "<button class='tablinks' onclick='openTab(\"Tab1\")' id='btnTab1'>Basic (S1)</button>";
  html += "<button class='tablinks' onclick='openTab(\"Tab2\")' id='btnTab2'>Toggle (S2)</button>";
  html += "<button class='tablinks' onclick='openTab(\"Tab3\")' id='btnTab3'>PWM (S3)</button>";
  html += "</div>";

  // =====================================================
  // TAB 1: S1 CONTENT (Basic Buttons)
  // =====================================================
  html += "<div id='Tab1' class='tabcontent'>";
  html += "<h3>Basic Control</h3>";
  
  html += "<h4>LED 1 (Blue)</h4>";
  html += "<p>Status: <b id='status1_tab1'>" + String(led1State ? "ON" : "OFF") + "</b></p>";
  html += "<button class='btn' onclick='controlLED(1, \"on\")'>Turn ON</button><br>";
  html += "<button class='btn btn-off' onclick='controlLED(1, \"off\")'>Turn OFF</button><hr>";

  html += "<h4>LED 2 (Red)</h4>";
  html += "<p>Status: <b id='status2_tab1'>" + String(led2State ? "ON" : "OFF") + "</b></p>";
  html += "<button class='btn' onclick='controlLED(2, \"on\")'>Turn ON</button><br>";
  html += "<button class='btn btn-off' onclick='controlLED(2, \"off\")'>Turn OFF</button><hr>";

  html += "<h4>LED 3 (Yellow)</h4>";
  html += "<p>Status: <b id='status3_tab1'>" + String(led3State ? "ON" : "OFF") + "</b></p>";
  html += "<button class='btn' onclick='controlLED(3, \"on\")'>Turn ON</button><br>";
  html += "<button class='btn btn-off' onclick='controlLED(3, \"off\")'>Turn OFF</button>";
  html += "</div>";

  // =====================================================
  // TAB 2: S2 CONTENT (Toggle Switches)
  // =====================================================
  html += "<div id='Tab2' class='tabcontent'>";
  html += "<h3>Toggle Control</h3>";

  html += "<div class='led-card'>";
  html += "<h2>LED 1 (Blue)</h2>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='toggle1' " + String(led1State ? "checked" : "") + " onchange='toggleLED(1)'>";
  html += "<span class='slider'></span></label>";
  html += "<p class='status'>Status: <b id='status1_tab2'>" + String(led1State ? "ON" : "OFF") + "</b></p>";
  html += "</div>";

  html += "<div class='led-card'>";
  html += "<h2>LED 2 (Red)</h2>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='toggle2' " + String(led2State ? "checked" : "") + " onchange='toggleLED(2)'>";
  html += "<span class='slider'></span></label>";
  html += "<p class='status'>Status: <b id='status2_tab2'>" + String(led2State ? "ON" : "OFF") + "</b></p>";
  html += "</div>";

  html += "<div class='led-card'>";
  html += "<h2>LED 3 (Yellow)</h2>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='toggle3' " + String(led3State ? "checked" : "") + " onchange='toggleLED(3)'>";
  html += "<span class='slider'></span></label>";
  html += "<p class='status'>Status: <b id='status3_tab2'>" + String(led3State ? "ON" : "OFF") + "</b></p>";
  html += "</div>";
  html += "</div>";

  // =====================================================
  // TAB 3: S3 CONTENT (PWM Slider)
  // =====================================================
  html += "<div id='Tab3' class='tabcontent'>";
  html += "<h3>PWM Control</h3>";
  html += "<div class='led-card'>";
  html += "<h2>LED 3 (Yellow)</h2>";
  html += "<div class='knob-container'>";
  html += "<div class='brightness-value' id='brightVal'>" + String(led3Brightness) + "</div>";
  html += "<div style='font-size:13px; color:#555;'>Brightness (0-255)</div>";
  html += "<input type='range' class='knob-range' id='brightSlider' min='0' max='255' value='" + String(led3Brightness) + "' oninput='updateKnob(this.value)' onchange='sendBrightness(this.value)'>";
  html += "<div style='font-size:13px; color:#555;'>&#9664; Dim &nbsp;&nbsp;&nbsp; Bright &#9654;</div>";
  html += "</div>";
  html += "<p class='status'>Status: <b id='status3_tab3'>" + String(led3State ? "ON" : "OFF") + "</b></p>";
  html += "</div>";
  html += "</div>";

  // =====================================================
  // JAVASCRIPT (AJAX & UI Sync)
  // =====================================================
  html += "<script>";
  html += "function openTab(tabName) {";
  html += "  var i, tabcontent, tablinks;";
  html += "  tabcontent = document.getElementsByClassName('tabcontent');";
  html += "  for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = 'none'; }";
  html += "  tablinks = document.getElementsByClassName('tablinks');";
  html += "  for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(' active', ''); }";
  html += "  document.getElementById(tabName).style.display = 'block';";
  html += "  document.getElementById('btn' + tabName).className += ' active';";
  html += "}";
  
  html += "const urlParams = new URLSearchParams(window.location.search);";
  html += "const activeTab = urlParams.get('tab') || 'Tab1';";
  html += "openTab(activeTab);";

  // Function to update UI elements across all tabs
  html += "function updateUI(led, state, brightness = null) {";
  html += "  let statusText = state ? 'ON' : 'OFF';";
  html += "  if(led === 1) {";
  html += "    document.getElementById('status1_tab1').innerText = statusText;";
  html += "    document.getElementById('status1_tab2').innerText = statusText;";
  html += "    document.getElementById('toggle1').checked = state;";
  html += "  }";
  html += "  if(led === 2) {";
  html += "    document.getElementById('status2_tab1').innerText = statusText;";
  html += "    document.getElementById('status2_tab2').innerText = statusText;";
  html += "    document.getElementById('toggle2').checked = state;";
  html += "  }";
  html += "  if(led === 3) {";
  html += "    document.getElementById('status3_tab1').innerText = statusText;";
  html += "    document.getElementById('status3_tab2').innerText = statusText;";
  html += "    document.getElementById('status3_tab3').innerText = statusText;";
  html += "    document.getElementById('toggle3').checked = state;";
  html += "    if(brightness !== null) {";
  html += "      document.getElementById('brightSlider').value = brightness;";
  html += "      document.getElementById('brightVal').innerText = brightness;";
  html += "    } else {";
  html += "      document.getElementById('brightSlider').value = state ? 255 : 0;";
  html += "      document.getElementById('brightVal').innerText = state ? 255 : 0;";
  html += "    }";
  html += "  }";
  html += "}";

  // AJAX calls for Basic Buttons
  html += "function controlLED(led, action) {";
  html += "  fetch('/led' + led + action)";
  html += "    .then(response => response.text())";
  html += "    .then(state => updateUI(led, state === '1'));";
  html += "}";

  // AJAX calls for Toggle Switches
  html += "function toggleLED(led) {";
  html += "  fetch('/led' + led + 'toggle')";
  html += "    .then(response => response.text())";
  html += "    .then(state => updateUI(led, state === '1'));";
  html += "}";

  // AJAX calls for PWM Slider
  html += "function updateKnob(val) {";
  html += "  document.getElementById('brightVal').innerText = val;";
  html += "}";
  html += "function sendBrightness(val) {";
  html += "  fetch('/led3brightness?value=' + val)";
  html += "    .then(() => updateUI(3, val > 0, val));";
  html += "}";
  
  // Polling to keep multiple devices in sync (optional, checks every 2 seconds)
  html += "setInterval(() => {";
  html += "  fetch('/status').then(res => res.json()).then(data => {";
  html += "    updateUI(1, data.led1);";
  html += "    updateUI(2, data.led2);";
  html += "    updateUI(3, data.led3, data.bright3);";
  html += "  });";
  html += "}, 2000);";

  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// =====================================================
// S1 HANDLERS (Basic Buttons)
// =====================================================
void handleLED1on() {
  digitalWrite(ledPin1, HIGH);
  led1State = true;
  server.send(200, "text/plain", "1");
}
void handleLED1off() {
  digitalWrite(ledPin1, LOW);
  led1State = false;
  server.send(200, "text/plain", "0");
}
void handleLED2on() {
  digitalWrite(ledPin2, HIGH);
  led2State = true;
  server.send(200, "text/plain", "1");
}
void handleLED2off() {
  digitalWrite(ledPin2, LOW);
  led2State = false;
  server.send(200, "text/plain", "0");
}
void handleLED3on() {
  ledcWrite(ledPin3, 255);
  led3State = true;
  led3Brightness = 255;
  server.send(200, "text/plain", "1");
}
void handleLED3off() {
  ledcWrite(ledPin3, 0);
  led3State = false;
  led3Brightness = 0;
  server.send(200, "text/plain", "0");
}

// =====================================================
// S2 HANDLERS (Toggle Switches)
// =====================================================
void handleLED1toggle() {
  led1State = !led1State;
  digitalWrite(ledPin1, led1State ? HIGH : LOW);
  server.send(200, "text/plain", led1State ? "1" : "0");
}
void handleLED2toggle() {
  led2State = !led2State;
  digitalWrite(ledPin2, led2State ? HIGH : LOW);
  server.send(200, "text/plain", led2State ? "1" : "0");
}
void handleLED3toggle() {
  led3State = !led3State;
  led3Brightness = led3State ? 255 : 0;
  ledcWrite(ledPin3, led3Brightness);
  server.send(200, "text/plain", led3State ? "1" : "0");
}

// =====================================================
// S3 HANDLER (PWM Slider)
// =====================================================
void handleLED3brightness() {
  if (server.hasArg("value")) {
    led3Brightness = server.arg("value").toInt();
    led3Brightness = constrain(led3Brightness, 0, 255);
    ledcWrite(ledPin3, led3Brightness);
    led3State = (led3Brightness > 0);
  }
  server.send(200, "text/plain", "OK");
}

// =====================================================
// STATUS HANDLER (For Polling)
// =====================================================
void handleStatus() {
  String json = "{";
  json += "\"led1\":" + String(led1State ? "true" : "false") + ",";
  json += "\"led2\":" + String(led2State ? "true" : "false") + ",";
  json += "\"led3\":" + String(led3State ? "true" : "false") + ",";
  json += "\"bright3\":" + String(led3Brightness);
  json += "}";
  server.send(200, "application/json", json);
}

// =====================================================
// SETUP & LOOP
// =====================================================

// setup() runs once when the ESP32 powers on or resets.
void setup() {
  Serial.begin(115200); // Start serial monitor at 115200 baud for debugging.
  
  // Set LED 1 and LED 2 pins as outputs and make sure they start OFF.
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);

  // Configure LED 3 for PWM: attach the pin with frequency and resolution,
  // and write 0 so it starts fully off.
  ledcAttach(ledPin3, pwmFreq, pwmResolution);
  ledcWrite(ledPin3, 0);

  // Connect to WiFi. The dots printed in the loop show it is still trying.
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Once connected, print the IP address. Open this address in a browser
  // on the same network to access the web interface.
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP().toString());
  Serial.flush(); // Wait for the serial data to finish sending
  delay(100);

  // Register each URL route with its handler function.
  // When the browser visits a URL, the matching function runs.
  server.on("/", handleRoot);
  
  // S1 Routes
  server.on("/led1on", handleLED1on);
  server.on("/led1off", handleLED1off);
  server.on("/led2on", handleLED2on);
  server.on("/led2off", handleLED2off);
  server.on("/led3on", handleLED3on);
  server.on("/led3off", handleLED3off);
  
  // S2 Routes
  server.on("/led1toggle", handleLED1toggle);
  server.on("/led2toggle", handleLED2toggle);
  server.on("/led3toggle", handleLED3toggle);
  
  // S3 Route
  server.on("/led3brightness", handleLED3brightness);

  // Status Route for Polling
  server.on("/status", handleStatus);

  server.begin(); // Start the web server.
  Serial.println("Web server started.");
}

// loop() runs continuously after setup().
// handleClient() checks if any browser has sent a request and responds to it.
void loop() {
  server.handleClient();
}
