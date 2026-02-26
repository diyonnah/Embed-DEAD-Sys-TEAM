#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Sol-Deco5";
const char* password = "Sol3d@dC";

WebServer server(80);

const int ledPin1 = 16;
const int ledPin2 = 18;
const int ledPin3 = 27;

// Track LED states
bool led1State = false;
bool led2State = false;
bool led3State = false;

void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP32 LED Control</title>";
  html += "<style>";
  html += "body { text-align:center; font-family:Arial; background:#f0f0f0; }";
  html += "h2 { color:#333; }";
  html += ".led-card { background:white; border-radius:12px; padding:20px; margin:15px auto; width:250px; box-shadow:0 2px 8px rgba(0,0,0,0.15); }";
  html += ".switch { position:relative; display:inline-block; width:60px; height:34px; }";
  html += ".switch input { opacity:0; width:0; height:0; }";
  html += ".slider { position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; background-color:#ccc; border-radius:34px; transition:.4s; }";
  html += ".slider:before { position:absolute; content:''; height:26px; width:26px; left:4px; bottom:4px; background-color:white; border-radius:50%; transition:.4s; }";
  html += "input:checked + .slider { background-color:#2196F3; }";
  html += "input:checked + .slider:before { transform:translateX(26px); }";
  html += ".status { font-size:14px; margin-top:8px; color:#555; }";
  html += "</style></head>";
  html += "<body>";
  html += "<h2>ESP32 LED Control</h2>";

  // LED 1 Blue
  html += "<div class='led-card'>";
  html += "<h2>LED 1 (Blue)</h2>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='led1' " + String(led1State ? "checked" : "") + " onchange=\"window.location.href='/led1toggle'\">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "<p class='status'>Status: <b>" + String(led1State ? "ON" : "OFF") + "</b></p>";
  html += "</div>";

  // LED 2 Red
  html += "<div class='led-card'>";
  html += "<h2>LED 2 (Red)</h2>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='led2' " + String(led2State ? "checked" : "") + " onchange=\"window.location.href='/led2toggle'\">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "<p class='status'>Status: <b>" + String(led2State ? "ON" : "OFF") + "</b></p>";
  html += "</div>";

  // LED 3 Yellow
  html += "<div class='led-card'>";
  html += "<h2>LED 3 (Yellow)</h2>";
  html += "<label class='switch'>";
  html += "<input type='checkbox' id='led3' " + String(led3State ? "checked" : "") + " onchange=\"window.location.href='/led3toggle'\">";
  html += "<span class='slider'></span>";
  html += "</label>";
  html += "<p class='status'>Status: <b>" + String(led3State ? "ON" : "OFF") + "</b></p>";
  html += "</div>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLED1toggle() {
  led1State = !led1State;
  digitalWrite(ledPin1, led1State ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLED2toggle() {
  led2State = !led2State;
  digitalWrite(ledPin2, led2State ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLED3toggle() {
  led3State = !led3State;
  digitalWrite(ledPin3, led3State ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPin3, LOW);

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
  server.on("/led1toggle", handleLED1toggle);
  server.on("/led2toggle", handleLED2toggle);
  server.on("/led3toggle", handleLED3toggle);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}