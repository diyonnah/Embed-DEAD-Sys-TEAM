#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Sol-Deco5";
const char* password = "Sol3d@dC";

WebServer server(80);

const int ledPin3 = 27;  // Yellow LED - PWM

// PWM brightness for Yellow LED
int led3Brightness = 0;

// PWM setup for ESP32
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;  // 8-bit = 0 to 255

void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP32 LED Control</title>";
  html += "<style>";
  html += "body { text-align:center; font-family:Arial; background:#f0f0f0; }";
  html += "h2 { color:#333; }";
  html += ".led-card { background:white; border-radius:12px; padding:20px; margin:15px auto; width:280px; box-shadow:0 2px 8px rgba(0,0,0,0.15); }";
  html += ".status { font-size:14px; margin-top:8px; color:#555; }";
  html += ".knob-container { display:flex; flex-direction:column; align-items:center; margin:10px 0; }";
  html += ".knob-label { font-size:13px; color:#555; margin-top:4px; }";
  html += ".brightness-value { font-size:22px; font-weight:bold; color:#FFA500; margin:5px 0; }";
  html += "input[type=range].knob-range { -webkit-appearance:none; width:200px; height:6px; border-radius:3px; background: linear-gradient(to right, #FFA500 0%, #FFA500 " + String(led3Brightness * 100 / 255) + "%, #ddd " + String(led3Brightness * 100 / 255) + "%, #ddd 100%); outline:none; margin:8px 0; }";
  html += "input[type=range].knob-range::-webkit-slider-thumb { -webkit-appearance:none; width:22px; height:22px; border-radius:50%; background: radial-gradient(circle at 35% 35%, #fff 10%, #FFA500 60%, #e08000 100%); box-shadow:0 2px 6px rgba(0,0,0,0.3); cursor:pointer; border:2px solid #e08000; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h2>ESP32 LED Control</h2>";

  // LED 3 Yellow - PWM with Potentiometer UI
  html += "<div class='led-card'>";
  html += "<h2>LED (Yellow)</h2>";
  html += "<div class='knob-container'>";
  html += "<div class='brightness-value' id='brightVal'>" + String(led3Brightness) + "</div>";
  html += "<div class='knob-label'>Brightness (0-255)</div>";
  html += "<input type='range' class='knob-range' id='brightSlider' min='0' max='255' value='" + String(led3Brightness) + "' oninput='updateKnob(this.value)' onchange='sendBrightness(this.value)'>";
  html += "<div class='knob-label'>&#9664; Dim &nbsp;&nbsp;&nbsp; Bright &#9654;</div>";
  html += "</div>";
  html += "<p class='status'>Status: <b id='ledStatus'>" + String(led3Brightness > 0 ? "ON" : "OFF") + "</b></p>";
  html += "</div>";

  // JavaScript
  html += "<script>";
  html += "function updateKnob(val) {";
  html += "  document.getElementById('brightVal').innerText = val;";
  html += "  var pct = Math.round(val / 255 * 100);";
  html += "  document.getElementById('brightSlider').style.background = 'linear-gradient(to right, #FFA500 0%, #FFA500 ' + pct + '%, #ddd ' + pct + '%, #ddd 100%)';";
  html += "}";
  html += "function sendBrightness(val) {";
  html += "  fetch('/led3brightness?value=' + val)";
  html += "    .then(() => { document.getElementById('ledStatus').innerText = val > 0 ? 'ON' : 'OFF'; });";
  html += "}";
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLED3brightness() {
  if (server.hasArg("value")) {
    led3Brightness = server.arg("value").toInt();
    led3Brightness = constrain(led3Brightness, 0, 255);
    ledcWrite(pwmChannel, led3Brightness);
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // Setup PWM for Yellow LED
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(ledPin3, pwmChannel);
  ledcWrite(pwmChannel, 0);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/led3brightness", handleLED3brightness);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}