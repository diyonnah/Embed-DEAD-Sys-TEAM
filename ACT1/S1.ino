#include <WiFi.h>
#include <WebServer.h>

// Replace with your WiFi credentials
const char* ssid = "Sol-Deco5";
const char* password = "Sol3d@dC";

// Create web server on port 80
WebServer server(80);

// Define LED pins
const int ledPin1 = 16;
const int ledPin2 = 18;
const int ledPin3 = 27;

// state
bool led1State = false;
bool led2State = false;
bool led3State = false;

// HTML page
void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP32 LED Control</title></head>";
  html += "<body style='text-align:center; font-family:Arial;'>";

  // LED 1 Blue
  html += "<h2>LED 1 (Blue) Control</h2>";
  html += "<p>Status: <b>" + String(led1State ? "ON" : "OFF") + "</b></p>";
  html += "<p><a href='/led1on'><button style='padding:10px 20px;'>Turn ON</button></a></p>";
  html += "<p><a href='/led1off'><button style='padding:10px 20px;'>Turn OFF</button></a></p>";

  // LED 2 Red
  html += "<h2>LED 2 (Red) Control</h2>";
  html += "<p>Status: <b>" + String(led2State ? "ON" : "OFF") + "</b></p>";
  html += "<p><a href='/led2on'><button style='padding:10px 20px;'>Turn ON</button></a></p>";
  html += "<p><a href='/led2off'><button style='padding:10px 20px;'>Turn OFF</button></a></p>";

  // LED 3 Yellow
  html += "<h2>LED 3 (Yellow) Control</h2>";
  html += "<p>Status: <b>" + String(led3State ? "ON" : "OFF") + "</b></p>";
  html += "<p><a href='/led3on'><button style='padding:10px 20px;'>Turn ON</button></a></p>";
  html += "<p><a href='/led3off'><button style='padding:10px 20px;'>Turn OFF</button></a></p>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLED1on() {
  digitalWrite(ledPin1, HIGH);
  led1State = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLED1off() {
  digitalWrite(ledPin1, LOW);
  led1State = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLED2on() {
  digitalWrite(ledPin2, HIGH);
  led2State = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLED2off() {
  digitalWrite(ledPin2, LOW);
  led2State = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLED3on() {
  digitalWrite(ledPin3, HIGH);
  led3State = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLED3off() {
  digitalWrite(ledPin3, LOW);
  led3State = false;
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
  server.on("/led1on", handleLED1on);
  server.on("/led1off", handleLED1off);
  server.on("/led2on", handleLED2on);
  server.on("/led2off", handleLED2off);
  server.on("/led3on", handleLED3on);
  server.on("/led3off", handleLED3off);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}