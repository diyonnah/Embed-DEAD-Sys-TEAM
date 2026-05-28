// ============================================
// TapIn - Smart Classroom Attendance System
// RFID + LCD + WiFi + Supabase
// ============================================

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===== PIN DEFINITIONS =====
#define SS_PIN 5
#define RST_PIN 0
#define GREEN_LED 35
#define RED_LED 32
#define BUZZER 34
#define SDA_PIN 21
#define SCL_PIN 22

// ===== CONFIGURATION - UPDATE THESE =====
const char* WIFI_SSID = "PLDTHOMEFIBR2a070";
const char* WIFI_PASSWORD = "PLDTWIFlfmc27";
const char* SUPABASE_URL = "https://zwlizayjnnqgeuzxeqex.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Inp3bGl6YXlqbm5xZ2V1enhlcWV4Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzkxMTQ3MTksImV4cCI6MjA5NDY5MDcxOX0.AHS4b-3YR9i_01AHgpw2rbaIi8-hLACIeOfChkMDsDM";

// Session ID and timing
String currentSessionId = "";
unsigned long sessionStartTime = 0;
const unsigned long ATTENDANCE_WINDOW_START = 0;
const unsigned long ATTENDANCE_WINDOW_LATE = 300;     // 5 min
const unsigned long ATTENDANCE_WINDOW_CLOSE = 600;    // 10 min

// ===== HARDWARE OBJECTS =====
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x27, 16, 2);
byte nuidPICC[4];
bool wifiConnected = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // ===== INITIALIZE GPIO =====
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  // ===== INITIALIZE LCD =====
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  // ===== INITIALIZE WIFI =====
  connectToWiFi();
  
  // ===== INITIALIZE RFID =====
  SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  // ===== START NEW SESSION =====
  if (wifiConnected) {
    startNewSession();
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TapIn Ready");
  Serial.println("System Ready - Tap card to begin");
  delay(500);
}

void loop() {
  // Check WiFi connection periodically
  if (!wifiConnected && millis() % 5000 == 0) {
    connectToWiFi();
  }
  
  if (!rfid.PICC_IsNewCardPresent())
    return;

  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.println(">>> Card Detected!");
  
  // ===== CARD TYPE VALIDATION =====
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    
    rejectCard("Bad Card");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // ===== DUPLICATE CARD CHECK =====
  if (rfid.uid.uidByte[0] == nuidPICC[0] && 
      rfid.uid.uidByte[1] == nuidPICC[1] && 
      rfid.uid.uidByte[2] == nuidPICC[2] && 
      rfid.uid.uidByte[3] == nuidPICC[3]) {
    
    Serial.println("Duplicate tap (ignoring)");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(500);
    return;
  }
  
  // Store card UID
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  
  // ===== GET CARD UID STRING =====
  String cardUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) cardUID += "0";
    cardUID += String(rfid.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();
  
  Serial.print("UID: ");
  Serial.println(cardUID);
  
  // ===== CALCULATE SEAT ASSIGNMENT =====
  int seat = ((rfid.uid.uidByte[0] + rfid.uid.uidByte[1] + 
               rfid.uid.uidByte[2] + rfid.uid.uidByte[3]) % 30) + 1;
  
  // ===== DETERMINE ATTENDANCE STATUS =====
  unsigned long elapsedTime = (millis() / 1000) - sessionStartTime;
  String status = "ABSENT";
  
  if (elapsedTime <= ATTENDANCE_WINDOW_LATE) {
    status = "PRESENT";
  } else if (elapsedTime <= ATTENDANCE_WINDOW_CLOSE) {
    status = "LATE";
  } else {
    status = "ABSENT";
  }
  
  // ===== DISPLAY ON LCD =====
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Seat: ");
  lcd.print(seat);
  lcd.print(" ");
  lcd.print(status);
  lcd.setCursor(0, 1);
  lcd.print("ID: ");
  lcd.print(cardUID);
  
  Serial.print("Seat: ");
  Serial.println(seat);
  Serial.print("Status: ");
  Serial.println(status);
  
  // ===== SEND TO SUPABASE =====
  if (wifiConnected && currentSessionId != "") {
    logAttendanceToSupabase(cardUID, seat, status);
  }
  
  // ===== SUCCESS FEEDBACK =====
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  delay(100);
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  digitalWrite(GREEN_LED, LOW);
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1000);
  
  // Return to ready state
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TapIn Ready");
}

// ===== HELPER FUNCTIONS =====

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }
  
  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\n✗ WiFi Failed");
    wifiConnected = false;
  }
}

void startNewSession() {
  Serial.println("Starting new session...");
  
  // Create a simple session ID (timestamp)
  currentSessionId = String(millis() / 1000);
  sessionStartTime = millis() / 1000;
  
  Serial.print("Session ID: ");
  Serial.println(currentSessionId);
  
  // TODO: Can create session record in Supabase if needed
}

void logAttendanceToSupabase(String cardUID, int seat, String status) {
  if (!wifiConnected) {
    Serial.println("No WiFi - skipping Supabase sync");
    return;
  }
  
  HTTPClient http;
  
  // Build the API endpoint
  String url = String(SUPABASE_URL) + "/rest/v1/attendance_logs";
  
  // Build JSON payload
  String jsonPayload = "{";
  jsonPayload += "\"session_id\":\"" + currentSessionId + "\",";
  jsonPayload += "\"card_uid\":\"" + cardUID + "\",";
  jsonPayload += "\"seat_number\":" + String(seat) + ",";
  jsonPayload += "\"attendance_status\":\"" + status + "\",";
  jsonPayload += "\"timestamp\":\"" + String(millis()) + "\"";
  jsonPayload += "}";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("Prefer", "return=minimal");
  
  Serial.println("Sending to Supabase...");
  Serial.println(jsonPayload);
  
  int httpResponseCode = http.POST(jsonPayload);
  
  if (httpResponseCode > 0) {
    Serial.print("✓ Response Code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("✗ Error: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

void rejectCard(String reason) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("REJECTED");
  lcd.setCursor(0, 1);
  lcd.print(reason);
  
  Serial.print("REJECT: ");
  Serial.println(reason);
  
  // Red LED + Buzzer
  digitalWrite(RED_LED, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(500);
  digitalWrite(BUZZER, LOW);
  digitalWrite(RED_LED, LOW);
  
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TapIn Ready");
}
