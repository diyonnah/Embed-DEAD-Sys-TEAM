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
#include <BLEDevice.h>
#include <time.h>

// ===== PIN DEFINITIONS =====
#define SS_PIN 5
#define RST_PIN 0
#define GREEN_LED 35
#define RED_LED 32
#define BUZZER 34
#define SDA_PIN 21
#define SCL_PIN 22

// ===== CONFIGURATION - UPDATE THESE =====
const char* WIFI_SSID = "CPE WIFI";
const char* WIFI_PASSWORD = "CP3Wi-Fi2025**";
const char* SUPABASE_URL = "https://rfgnfkpmnopptwmmlizf.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InJmZ25ma3Btbm9wcHR3bW1saXpmIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Nzk5NjMxMjAsImV4cCI6MjA5NTUzOTEyMH0.8w1r9VWI2ybdcFJBRtewlwXNeTGRxpVY-FHbAqUpudc";
const char* SUPABASE_STUDENTS_TABLE = "students"; // Must match your Supabase table name
const char* SUPABASE_SESSIONS_TABLE = "sessions";

// BLE settings
const uint32_t BLE_SCAN_DURATION_SEC = 10; // Adjust to 10-15 seconds if needed
const int BLE_RSSI_MIN = -75; // Reject weaker signals; adjust per room size

// Session and time settings
const long TIMEZONE_OFFSET_SEC = 8 * 3600; // UTC+8
const long DAYLIGHT_OFFSET_SEC = 0;
const unsigned long SESSION_SYNC_INTERVAL_MS = 30000;
const unsigned long ATTENDANCE_LATE_WINDOW_SEC = 300; // 5 min from session start
const unsigned long IDLE_DISPLAY_INTERVAL_MS = 2000;

String activeSessionId = "";
time_t sessionStartEpoch = 0;
time_t sessionEndEpoch = 0;
unsigned long lastSessionSyncMs = 0;
unsigned long lastIdleDisplayMs = 0;
bool hasActiveSession = false;

// ===== HARDWARE OBJECTS =====
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x27, 16, 2);
byte nuidPICC[4];
bool wifiConnected = false;
BLEScan* bleScan = nullptr;

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

  // ===== INITIALIZE BLE SCANNER =====
  BLEDevice::init("");
  bleScan = BLEDevice::getScan();
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(80);
  
  // ===== INITIALIZE TIME =====
  if (wifiConnected) {
    initTimeSync();
    syncActiveSession();
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

  if (wifiConnected && millis() - lastSessionSyncMs >= SESSION_SYNC_INTERVAL_MS) {
    syncActiveSession();
  }

  updateIdleDisplay();
  
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

  if (!wifiConnected) {
    rejectCard("No WiFi");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // ===== FETCH STUDENT BLE UUID + SEAT =====
  String expectedBleId = "";
  int seat = -1;
  if (!fetchStudentInfo(cardUID, expectedBleId, seat)) {
    rejectCard("Unknown Card");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  if (!hasActiveSession) {
    rejectCard("No Session");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  time_t nowEpoch = getNowEpoch();
  if (nowEpoch == 0) {
    rejectCard("No Time");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // ===== BLE PROXIMITY VERIFICATION =====
  if (!verifyBleProximity(expectedBleId)) {
    rejectCard("Phone Not Detected");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  
  // ===== DETERMINE ATTENDANCE STATUS =====
  String status = "ABSENT";

  if (nowEpoch < sessionStartEpoch) {
    rejectCard("Not Started");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  time_t lateCutoff = sessionStartEpoch + ATTENDANCE_LATE_WINDOW_SEC;
  if (nowEpoch <= lateCutoff) {
    status = "PRESENT";
  } else if (nowEpoch <= sessionEndEpoch) {
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
  if (wifiConnected && activeSessionId != "") {
    logAttendanceToSupabase(cardUID, expectedBleId, seat, status, nowEpoch);
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

void updateIdleDisplay() {
  if (millis() - lastIdleDisplayMs < IDLE_DISPLAY_INTERVAL_MS) {
    return;
  }

  lastIdleDisplayMs = millis();
  lcd.clear();
  lcd.setCursor(0, 0);

  if (!hasActiveSession) {
    lcd.print("No Active");
    lcd.setCursor(0, 1);
    lcd.print("Session");
  } else {
    lcd.print("TapIn Ready");
  }
}

void logAttendanceToSupabase(String cardUID, String bleId, int seat, String status, time_t timestamp) {
  if (!wifiConnected) {
    Serial.println("No WiFi - skipping Supabase sync");
    return;
  }
  
  HTTPClient http;
  
  // Build the API endpoint
  String url = String(SUPABASE_URL) + "/rest/v1/attendance_logs";
  
  // Build JSON payload
  String jsonPayload = "{";
  jsonPayload += "\"session_id\":\"" + activeSessionId + "\",";
  jsonPayload += "\"card_uid\":\"" + cardUID + "\",";
  jsonPayload += "\"ble_id\":\"" + bleId + "\",";
  jsonPayload += "\"seat_number\":" + String(seat) + ",";
  jsonPayload += "\"attendance_status\":\"" + status + "\",";
  jsonPayload += "\"timestamp\":\"" + String((long)timestamp) + "\"";
  jsonPayload += "}";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("apikey", String(SUPABASE_KEY));
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

void initTimeSync() {
  configTime(TIMEZONE_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "PHT-8", 1);
  tzset();
}

time_t getNowEpoch() {
  time_t now = time(nullptr);
  if (now < 100000) {
    return 0;
  }
  return now;
}

time_t parseSessionTime(const String& isoTime) {
  if (isoTime.length() == 0) {
    return 0;
  }

  String normalized = isoTime;
  normalized.replace('T', ' ');

  struct tm timeInfo;
  memset(&timeInfo, 0, sizeof(timeInfo));

  const char* formats[] = {"%Y-%m-%d %H:%M:%S", "%Y-%m-%d %H:%M"};
  for (size_t i = 0; i < 2; i++) {
    char* parsed = strptime(normalized.c_str(), formats[i], &timeInfo);
    if (parsed != nullptr) {
      return mktime(&timeInfo);
    }
  }

  return 0;
}

void syncActiveSession() {
  lastSessionSyncMs = millis();
  String sessionId = "";
  time_t startEpoch = 0;
  time_t endEpoch = 0;

  if (!fetchActiveSession(sessionId, startEpoch, endEpoch)) {
    hasActiveSession = false;
    activeSessionId = "";
    sessionStartEpoch = 0;
    sessionEndEpoch = 0;
    return;
  }

  activeSessionId = sessionId;
  sessionStartEpoch = startEpoch;
  sessionEndEpoch = endEpoch;
  hasActiveSession = true;
}

bool fetchActiveSession(String& sessionIdOut, time_t& startOut, time_t& endOut) {
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/" + SUPABASE_SESSIONS_TABLE +
               "?status=eq.active&select=id,start_time,end_time&order=start_time.desc&limit=1";

  http.begin(url);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("apikey", String(SUPABASE_KEY));

  int httpResponseCode = http.GET();
  if (httpResponseCode <= 0) {
    Serial.print("Session lookup failed: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err || !doc.is<JsonArray>() || doc.size() == 0) {
    return false;
  }

  JsonObject session = doc[0];
  if (!session.containsKey("id") || !session.containsKey("start_time") || !session.containsKey("end_time")) {
    return false;
  }

  sessionIdOut = String((const char*)session["id"]);
  startOut = parseSessionTime(String((const char*)session["start_time"]));
  endOut = parseSessionTime(String((const char*)session["end_time"]));

  return sessionIdOut.length() > 0 && startOut > 0 && endOut > 0;
}

bool fetchStudentInfo(const String& cardUID, String& bleIdOut, int& seatOut) {
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/" + SUPABASE_STUDENTS_TABLE +
               "?card_uid=eq." + cardUID + "&select=ble_id,seat_number";

  http.begin(url);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("apikey", String(SUPABASE_KEY));

  int httpResponseCode = http.GET();
  if (httpResponseCode <= 0) {
    Serial.print("Student lookup failed: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("Student JSON parse error");
    return false;
  }

  if (!doc.is<JsonArray>() || doc.size() == 0) {
    return false;
  }

  JsonObject student = doc[0];
  if (!student.containsKey("ble_id") || !student.containsKey("seat_number")) {
    return false;
  }

  bleIdOut = String((const char*)student["ble_id"]);
  seatOut = student["seat_number"].as<int>();
  return bleIdOut.length() > 0 && seatOut > 0;
}

bool verifyBleProximity(const String& expectedUuid) {
  if (expectedUuid.length() == 0 || bleScan == nullptr) {
    return false;
  }

  Serial.println("Scanning BLE...");
  BLEScanResults* results = bleScan->start(BLE_SCAN_DURATION_SEC, false);
  bool found = false;

  if (results == nullptr) {
    return false;
  }

  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice device = results->getDevice(i);
    if (!device.haveServiceUUID()) {
      continue;
    }

    if (device.getRSSI() < BLE_RSSI_MIN) {
      continue;
    }

    BLEUUID serviceUuid = device.getServiceUUID();
    String serviceUuidStr = String(serviceUuid.toString().c_str());
    serviceUuidStr.toUpperCase();

    String expectedUpper = expectedUuid;
    expectedUpper.toUpperCase();

    if (serviceUuidStr == expectedUpper) {
      found = true;
      break;
    }
  }

  bleScan->clearResults();
  Serial.println(found ? "BLE verified" : "BLE not found");
  return found;
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
  updateIdleDisplay();
}
