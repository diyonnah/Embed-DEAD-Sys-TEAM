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
bool led1State = false;
bool led2State = false;
bool led3State = false;

// PWM settings for LED 3 (Yellow).
int led3Brightness = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;  // 8-bit = 0 to 255

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 LED Control</title>";
  html += "<style>";

  // ── Google Font ──
  html += "@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap');";

  // ── Design Tokens ──
  html += ":root {";
  html += "--bg: #04080f;";
  html += "--surface: rgba(255,255,255,0.05);";
  html += "--surface2: rgba(255,255,255,0.08);";
  html += "--border: rgba(255,255,255,0.08);";
  html += "--border-soft: rgba(255,255,255,0.13);";
  html += "--text-1: #f1f5f9;";
  html += "--text-2: #94a3b8;";
  html += "--text-3: #4b6080;";
  html += "--accent: #3b82f6;";
  html += "--accent-dim: rgba(59,130,246,0.22);";
  html += "--green: #22c55e;";
  html += "--green-dim: rgba(34,197,94,0.14);";
  html += "--red: #ef4444;";
  html += "--red-dim: rgba(239,68,68,0.14);";
  html += "--amber: #f59e0b;";
  html += "--amber-dim: rgba(245,158,11,0.18);";
  html += "--radius: 16px;";
  html += "--glass-blur: blur(20px) saturate(160%);";
  html += "--card-shadow: 0 4px 32px rgba(0,0,0,0.45), inset 0 1px 0 rgba(255,255,255,0.06);";
  html += "}";

  // ── Reset & Base ──
  html += "*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body {";
  html += "font-family: 'Inter', 'Segoe UI', sans-serif;";
  html += "text-align: center;";
  html += "background: var(--bg);";
  html += "background-image: radial-gradient(ellipse 120% 55% at 50% 0%, rgba(37,99,235,0.45) 0%, transparent 70%), radial-gradient(ellipse 60% 30% at 80% 0%, rgba(99,102,241,0.2) 0%, transparent 60%);";
  html += "background-attachment: fixed;";
  html += "color: var(--text-1);";
  html += "min-height: 100vh;";
  html += "}";

  // ── App Header ──
  html += ".app-header {";
  html += "background: rgba(10,20,45,0.55);";
  html += "backdrop-filter: var(--glass-blur);";
  html += "-webkit-backdrop-filter: var(--glass-blur);";
  html += "border-bottom: 1px solid var(--border);";
  html += "padding: 20px max(20px, env(safe-area-inset-right)) 18px max(20px, env(safe-area-inset-left));";
  html += "padding-top: calc(18px + env(safe-area-inset-top));";
  html += "position: relative;";
  html += "}";
  html += ".app-title { font-size: 19px; font-weight: 800; color: var(--text-1); letter-spacing: -0.4px; }";
  html += ".app-subtitle { font-size: 11px; color: rgba(148,163,184,0.8); letter-spacing: 0.6px; margin-top: 4px; max-width: 280px; margin-left: auto; margin-right: auto; line-height: 1.5; }";
  html += ".device-chip { display: inline-flex; align-items: center; gap: 6px; background: rgba(255,255,255,0.07); backdrop-filter: blur(8px); -webkit-backdrop-filter: blur(8px); border: 1px solid rgba(255,255,255,0.1); border-radius: 20px; padding: 5px 12px; font-size: 11px; font-weight: 600; color: var(--text-2); margin-top: 12px; letter-spacing: 0.3px; }";
  html += ".device-chip::before { content: ''; width: 7px; height: 7px; border-radius: 50%; background: var(--green); box-shadow: 0 0 8px var(--green); animation: pulse 2.5s infinite; }";
  html += "@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.4; } }";

  // ── Tab Bar ──
  html += ".tab { display: flex; background: rgba(8,14,28,0.5); backdrop-filter: var(--glass-blur); -webkit-backdrop-filter: var(--glass-blur); border-bottom: 1px solid var(--border); padding-left: env(safe-area-inset-left); padding-right: env(safe-area-inset-right); }";
  html += ".tab button { flex: 1; background: transparent; border: none; border-bottom: 2px solid transparent; outline: none; cursor: pointer; padding: 16px 8px 14px; min-height: 52px; font-size: 12px; font-weight: 700; font-family: inherit; color: var(--text-3); letter-spacing: 0.6px; text-transform: uppercase; transition: color 0.2s, background 0.2s; }";
  html += ".tab button:hover { color: var(--text-2); background: rgba(255,255,255,0.02); }";
  html += ".tab button.active { color: var(--accent); border-bottom: 2px solid var(--accent); background: transparent; }";

  // ── Tab Content ──
  html += ".tabcontent { display: none; padding: 24px max(16px, env(safe-area-inset-right)) 48px max(16px, env(safe-area-inset-left)); background: transparent; min-height: 420px; }";
  html += ".section-label { font-size: 10px; font-weight: 700; color: var(--text-3); text-transform: uppercase; letter-spacing: 2px; margin-bottom: 20px; padding-top: 6px; }";

  // ── Control Cards ──
  html += ".control-card { background: rgba(255,255,255,0.05); backdrop-filter: var(--glass-blur); -webkit-backdrop-filter: var(--glass-blur); border: 1px solid rgba(255,255,255,0.09); border-radius: var(--radius); padding: 20px 20px 18px; margin: 12px auto; width: 100%; max-width: 480px; text-align: left; box-shadow: var(--card-shadow); transition: border-color 0.25s, box-shadow 0.25s, background 0.25s; }";
  html += ".control-card:hover { background: rgba(255,255,255,0.07); border-color: rgba(255,255,255,0.15); box-shadow: 0 8px 40px rgba(0,0,0,0.55), inset 0 1px 0 rgba(255,255,255,0.09); }";
  html += ".card-header { display: flex; align-items: center; justify-content: space-between; margin-bottom: 14px; }";
  html += ".card-title { display: flex; align-items: center; gap: 9px; font-size: 14px; font-weight: 700; color: var(--text-1); }";

  // ── LED Indicator Dots ──
  html += ".led-dot { width: 10px; height: 10px; border-radius: 50%; flex-shrink: 0; transition: background 0.4s ease, box-shadow 0.4s ease; }";
  html += ".led-dot.blue  { background: #0f2540; box-shadow: none; }";
  html += ".led-dot.red   { background: #3b0f0f; box-shadow: none; }";
  html += ".led-dot.amber { background: #2e1d04; box-shadow: none; }";
  html += ".led-dot.blue.lit  { background: #60a5fa; box-shadow: 0 0 6px 1px rgba(96,165,250,0.9), 0 0 16px rgba(96,165,250,0.45); }";
  html += ".led-dot.red.lit   { background: #f87171; box-shadow: 0 0 6px 1px rgba(248,113,113,0.9), 0 0 16px rgba(248,113,113,0.45); }";
  html += ".led-dot.amber.lit { background: #fbbf24; box-shadow: 0 0 6px 1px rgba(251,191,36,0.9), 0 0 16px rgba(251,191,36,0.45); }";

  // ── Status Badges ──
  html += ".badge { display: inline-block; padding: 3px 9px; border-radius: 20px; font-size: 10px; font-weight: 700; letter-spacing: 0.8px; text-transform: uppercase; transition: background 0.3s, color 0.3s, border-color 0.3s; }";
  html += ".badge-on  { background: var(--green-dim); color: var(--green); border: 1px solid rgba(34,197,94,0.3); }";
  html += ".badge-off { background: rgba(75,94,122,0.1); color: var(--text-3); border: 1px solid rgba(75,94,122,0.2); }";

  // ── S1 Button Row ──
  html += ".btn-row { display: flex; gap: 10px; }";
  html += ".btn { flex: 1; padding: 14px 10px; min-height: 52px; font-size: 14px; font-weight: 600; font-family: inherit; border-radius: 10px; border: none; cursor: pointer; letter-spacing: 0.2px; transition: transform 0.12s, filter 0.12s, box-shadow 0.12s; }";
  html += ".btn-on  { background: linear-gradient(145deg, #1d4ed8, #3b82f6); color: #fff; box-shadow: 0 3px 12px rgba(59,130,246,0.32); }";
  html += ".btn-on:hover  { filter: brightness(1.12); transform: translateY(-1px); box-shadow: 0 5px 18px rgba(59,130,246,0.48); }";
  html += ".btn-off { background: linear-gradient(145deg, #991b1b, #ef4444); color: #fff; box-shadow: 0 3px 12px rgba(239,68,68,0.28); }";
  html += ".btn-off:hover { filter: brightness(1.1); transform: translateY(-1px); box-shadow: 0 5px 18px rgba(239,68,68,0.42); }";
  html += ".btn:active { transform: translateY(0); filter: brightness(0.9); }";

  // ── Toggle Switch ──
  html += ".toggle-row { display: flex; align-items: center; justify-content: space-between; min-height: 52px; padding: 4px 0; }";
  html += ".switch { position: relative; display: inline-block; width: 56px; height: 32px; flex-shrink: 0; }";
  html += ".switch input { opacity: 0; width: 0; height: 0; }";
  html += ".slider { position: absolute; cursor: pointer; inset: 0; background: rgba(255,255,255,0.07); border: 1px solid rgba(255,255,255,0.12); border-radius: 32px; transition: background 0.3s, border-color 0.3s; }";
  html += ".slider::before { position: absolute; content: ''; height: 22px; width: 22px; left: 4px; bottom: 4px; background: var(--text-3); border-radius: 50%; transition: transform 0.3s, background 0.3s; box-shadow: 0 2px 4px rgba(0,0,0,0.5); }";
  html += "input:checked + .slider { background: var(--accent-dim); border-color: var(--accent); }";
  html += "input:checked + .slider::before { transform: translateX(24px); background: var(--accent); box-shadow: 0 0 8px rgba(59,130,246,0.5); }";

  // ── S3 PWM ──
  html += ".pwm-display { background: rgba(255,255,255,0.04); backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px); border: 1px solid rgba(255,255,255,0.08); border-radius: 12px; padding: 16px 12px 14px; margin: 4px 0 10px; text-align: center; }";
  html += ".brightness-value { font-size: 56px; font-weight: 800; color: var(--amber); line-height: 1; letter-spacing: -3px; font-variant-numeric: tabular-nums; text-shadow: 0 0 30px rgba(245,158,11,0.4); }";
  html += ".brightness-label { font-size: 10px; font-weight: 600; color: var(--text-3); text-transform: uppercase; letter-spacing: 1.2px; margin-top: 5px; }";
  html += ".knob-container { display: flex; flex-direction: column; align-items: stretch; margin-top: 6px; }";
  html += ".slider-labels { display: flex; justify-content: space-between; font-size: 10px; font-weight: 600; color: var(--text-3); letter-spacing: 0.5px; margin-top: 4px; text-transform: uppercase; }";
  html += "input[type=range].knob-range { -webkit-appearance: none; width: 100%; height: 6px; border-radius: 4px; background: var(--border); outline: none; margin: 16px 0 0; cursor: pointer; touch-action: none; }";
  html += "input[type=range].knob-range::-webkit-slider-thumb { -webkit-appearance: none; width: 28px; height: 28px; border-radius: 50%; background: #fff; border: 3px solid var(--amber); cursor: pointer; box-shadow: 0 0 14px rgba(245,158,11,0.55), 0 2px 4px rgba(0,0,0,0.4); transition: box-shadow 0.2s, transform 0.15s; }";
  html += "input[type=range].knob-range::-webkit-slider-thumb:hover { box-shadow: 0 0 22px rgba(245,158,11,0.8), 0 2px 6px rgba(0,0,0,0.4); transform: scale(1.1); }";

  // ── Footer ──
  html += ".app-footer { padding: 18px max(20px, env(safe-area-inset-right)) calc(18px + env(safe-area-inset-bottom)) max(20px, env(safe-area-inset-left)); font-size: 10px; font-weight: 500; color: var(--text-3); background: rgba(8,14,28,0.4); backdrop-filter: var(--glass-blur); -webkit-backdrop-filter: var(--glass-blur); border-top: 1px solid var(--border); letter-spacing: 0.8px; text-transform: uppercase; }";

  html += "</style></head><body>";

  // ── App Header ──
  html += "<div class='app-header'>";
  html += "<div class='app-title'>Laboratory 1</div>";
  html += "<div class='app-subtitle'>Controlling an LED using NodeMCU over Wi-Fi</div>";
  html += "<div class='device-chip'>ESP32 WROOM &nbsp;&middot;&nbsp; WebServer</div>";
  html += "</div>";

  // ── Tab Bar ──
  html += "<div class='tab'>";
  html += "<button class='tablinks' onclick='openTab(\"Tab1\")' id='btnTab1'>Basic</button>";
  html += "<button class='tablinks' onclick='openTab(\"Tab2\")' id='btnTab2'>Toggle</button>";
  html += "<button class='tablinks' onclick='openTab(\"Tab3\")' id='btnTab3'>PWM</button>";
  html += "</div>";

  // =====================================================
  // TAB 1: S1 CONTENT (Basic Buttons)
  // =====================================================
  html += "<div id='Tab1' class='tabcontent'>";
  html += "<p class='section-label'>Basic Control</p>";

  html += "<div class='control-card'>";
  html += "<div class='card-header'>";
  html += "<div class='card-title'><span class='led-dot blue" + String(led1State ? " lit" : "") + "' data-led='1'></span>LED 1 &mdash; Blue</div>";
  html += "<span class='badge " + String(led1State ? "badge-on" : "badge-off") + "' id='status1_tab1'>" + String(led1State ? "ON" : "OFF") + "</span>";
  html += "</div>";
  html += "<div class='btn-row'>";
  html += "<button class='btn btn-on' onclick='controlLED(1, \"on\")'>Turn ON</button>";
  html += "<button class='btn btn-off' onclick='controlLED(1, \"off\")'>Turn OFF</button>";
  html += "</div></div>";

  html += "<div class='control-card'>";
  html += "<div class='card-header'>";
  html += "<div class='card-title'><span class='led-dot red" + String(led2State ? " lit" : "") + "' data-led='2'></span>LED 2 &mdash; Red</div>";
  html += "<span class='badge " + String(led2State ? "badge-on" : "badge-off") + "' id='status2_tab1'>" + String(led2State ? "ON" : "OFF") + "</span>";
  html += "</div>";
  html += "<div class='btn-row'>";
  html += "<button class='btn btn-on' onclick='controlLED(2, \"on\")'>Turn ON</button>";
  html += "<button class='btn btn-off' onclick='controlLED(2, \"off\")'>Turn OFF</button>";
  html += "</div></div>";

  html += "<div class='control-card'>";
  html += "<div class='card-header'>";
  html += "<div class='card-title'><span class='led-dot amber" + String(led3State ? " lit" : "") + "' data-led='3'></span>LED 3 &mdash; Yellow</div>";
  html += "<span class='badge " + String(led3State ? "badge-on" : "badge-off") + "' id='status3_tab1'>" + String(led3State ? "ON" : "OFF") + "</span>";
  html += "</div>";
  html += "<div class='btn-row'>";
  html += "<button class='btn btn-on' onclick='controlLED(3, \"on\")'>Turn ON</button>";
  html += "<button class='btn btn-off' onclick='controlLED(3, \"off\")'>Turn OFF</button>";
  html += "</div></div>";

  html += "</div>"; // end Tab1

  // =====================================================
  // TAB 2: S2 CONTENT (Toggle Switches)
  // =====================================================
  html += "<div id='Tab2' class='tabcontent'>";
  html += "<p class='section-label'>Toggle Control</p>";

  html += "<div class='control-card'>";
  html += "<div class='card-header'><div class='card-title'><span class='led-dot blue" + String(led1State ? " lit" : "") + "' data-led='1'></span>LED 1 &mdash; Blue</div></div>";
  html += "<div class='toggle-row'>";
  html += "<span class='badge " + String(led1State ? "badge-on" : "badge-off") + "' id='status1_tab2'>" + String(led1State ? "ON" : "OFF") + "</span>";
  html += "<label class='switch'><input type='checkbox' id='toggle1' " + String(led1State ? "checked" : "") + " onchange='toggleLED(1)'><span class='slider'></span></label>";
  html += "</div></div>";

  html += "<div class='control-card'>";
  html += "<div class='card-header'><div class='card-title'><span class='led-dot red" + String(led2State ? " lit" : "") + "' data-led='2'></span>LED 2 &mdash; Red</div></div>";
  html += "<div class='toggle-row'>";
  html += "<span class='badge " + String(led2State ? "badge-on" : "badge-off") + "' id='status2_tab2'>" + String(led2State ? "ON" : "OFF") + "</span>";
  html += "<label class='switch'><input type='checkbox' id='toggle2' " + String(led2State ? "checked" : "") + " onchange='toggleLED(2)'><span class='slider'></span></label>";
  html += "</div></div>";

  html += "<div class='control-card'>";
  html += "<div class='card-header'><div class='card-title'><span class='led-dot amber" + String(led3State ? " lit" : "") + "' data-led='3'></span>LED 3 &mdash; Yellow</div></div>";
  html += "<div class='toggle-row'>";
  html += "<span class='badge " + String(led3State ? "badge-on" : "badge-off") + "' id='status3_tab2'>" + String(led3State ? "ON" : "OFF") + "</span>";
  html += "<label class='switch'><input type='checkbox' id='toggle3' " + String(led3State ? "checked" : "") + " onchange='toggleLED(3)'><span class='slider'></span></label>";
  html += "</div></div>";

  html += "</div>"; // end Tab2

  // =====================================================
  // TAB 3: S3 CONTENT (PWM Slider)
  // =====================================================
  html += "<div id='Tab3' class='tabcontent'>";
  html += "<p class='section-label'>PWM Control</p>";
  html += "<div class='control-card'>";
  html += "<div class='card-header'>";
  html += "<div class='card-title'><span class='led-dot amber" + String(led3State ? " lit" : "") + "' data-led='3'></span>LED 3 &mdash; Yellow</div>";
  html += "<span class='badge " + String(led3State ? "badge-on" : "badge-off") + "' id='status3_tab3'>" + String(led3State ? "ON" : "OFF") + "</span>";
  html += "</div>";
  html += "<div class='pwm-display'>";
  html += "<div class='brightness-value' id='brightVal'>" + String(led3Brightness) + "</div>";
  html += "<div class='brightness-label'>Brightness (0 &ndash; 255)</div>";
  html += "</div>";
  html += "<div class='knob-container'>";
  html += "<input type='range' class='knob-range' id='brightSlider' min='0' max='255' value='" + String(led3Brightness) + "' oninput='updateKnob(this.value)' onchange='sendBrightness(this.value)'>";
  html += "<div class='slider-labels'><span>Dim</span><span>Bright</span></div>";
  html += "</div></div>";
  html += "</div>"; // end Tab3

  html += "<div class='app-footer'>ESP32 WROOM &nbsp;&bull;&nbsp; GPIO Control &nbsp;&bull;&nbsp; Polling every 2s</div>";

  // =====================================================
  // JAVASCRIPT (AJAX & UI Sync)
  // =====================================================
  html += "<script>";

  html += "function openTab(tabName) {";
  html += "var i, tabcontent, tablinks;";
  html += "tabcontent = document.getElementsByClassName('tabcontent');";
  html += "for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = 'none'; }";
  html += "tablinks = document.getElementsByClassName('tablinks');";
  html += "for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(' active', ''); }";
  html += "document.getElementById(tabName).style.display = 'block';";
  html += "document.getElementById('btn' + tabName).className += ' active';";
  html += "}";

  html += "const urlParams = new URLSearchParams(window.location.search);";
  html += "const activeTab = urlParams.get('tab') || 'Tab1';";
  html += "openTab(activeTab);";
  html += "updateKnob(document.getElementById('brightSlider').value);";

  // Sets badge text and colour class
  html += "function setBadge(id, state) {";
  html += "const el = document.getElementById(id);";
  html += "if (!el) return;";
  html += "el.innerText = state ? 'ON' : 'OFF';";
  html += "el.className = 'badge ' + (state ? 'badge-on' : 'badge-off');";
  html += "}";

  // Toggle lit class on every dot for this LED
  html += "function setDots(led, state) {";
  html += "document.querySelectorAll('[data-led=\"' + led + '\"]').forEach(dot => {";
  html += "dot.classList.toggle('lit', state);";
  html += "});";
  html += "}";

  // Update UI across all tabs
  html += "function updateUI(led, state, brightness = null) {";
  html += "setDots(led, state);";
  html += "if (led === 1) {";
  html += "setBadge('status1_tab1', state);";
  html += "setBadge('status1_tab2', state);";
  html += "document.getElementById('toggle1').checked = state;";
  html += "}";
  html += "if (led === 2) {";
  html += "setBadge('status2_tab1', state);";
  html += "setBadge('status2_tab2', state);";
  html += "document.getElementById('toggle2').checked = state;";
  html += "}";
  html += "if (led === 3) {";
  html += "setBadge('status3_tab1', state);";
  html += "setBadge('status3_tab2', state);";
  html += "setBadge('status3_tab3', state);";
  html += "document.getElementById('toggle3').checked = state;";
  html += "const bright = brightness !== null ? brightness : (state ? 255 : 0);";
  html += "document.getElementById('brightSlider').value = bright;";
  html += "updateKnob(bright);";
  html += "}";
  html += "}";

  // S1 – Basic Buttons
  html += "function controlLED(led, action) {";
  html += "fetch('/led' + led + action)";
  html += ".then(response => response.text())";
  html += ".then(state => updateUI(led, state === '1'));";
  html += "}";

  // S2 – Toggle Switches
  html += "function toggleLED(led) {";
  html += "fetch('/led' + led + 'toggle')";
  html += ".then(response => response.text())";
  html += ".then(state => updateUI(led, state === '1'));";
  html += "}";

  // S3 – PWM Slider
  html += "function updateKnob(val) {";
  html += "document.getElementById('brightVal').innerText = val;";
  html += "const pct = Math.round(val / 255 * 100);";
  html += "document.getElementById('brightSlider').style.background = 'linear-gradient(to right, #f59e0b ' + pct + '%, rgba(255,255,255,0.08) ' + pct + '%)';";
  html += "}";
  html += "function sendBrightness(val) {";
  html += "fetch('/led3brightness?value=' + val)";
  html += ".then(() => updateUI(3, val > 0, val));";
  html += "}";

  // Polling every 2 seconds to sync multiple devices
  html += "setInterval(() => {";
  html += "fetch('/status').then(res => res.json()).then(data => {";
  html += "updateUI(1, data.led1);";
  html += "updateUI(2, data.led2);";
  html += "updateUI(3, data.led3, data.bright3);";
  html += "});";
  html += "}, 2000);";

  html += "</script></body></html>";
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
void setup() {
  Serial.begin(115200);

  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);

  ledcAttach(ledPin3, pwmFreq, pwmResolution);
  ledcWrite(ledPin3, 0);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP().toString());
  Serial.flush();
  delay(100);

  // Register URL routes
  server.on("/", handleRoot);

  // S1 Routes
  server.on("/led1on",  handleLED1on);
  server.on("/led1off", handleLED1off);
  server.on("/led2on",  handleLED2on);
  server.on("/led2off", handleLED2off);
  server.on("/led3on",  handleLED3on);
  server.on("/led3off", handleLED3off);

  // S2 Routes
  server.on("/led1toggle", handleLED1toggle);
  server.on("/led2toggle", handleLED2toggle);
  server.on("/led3toggle", handleLED3toggle);

  // S3 Route
  server.on("/led3brightness", handleLED3brightness);

  // Status Route
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
