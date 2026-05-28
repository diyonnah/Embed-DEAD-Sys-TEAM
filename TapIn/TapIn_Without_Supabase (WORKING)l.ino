// ============================================
// TapIn - Smart Classroom Attendance System
// RFID + LCD (Minimal - BLE/WiFi in v2)
// ============================================

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== PIN DEFINITIONS =====
#define SS_PIN 5
#define RST_PIN 0
#define GREEN_LED 35
#define RED_LED 32
#define BUZZER 34
#define SDA_PIN 21
#define SCL_PIN 22

// ===== CONFIGURATION =====
unsigned long sessionStartTime = 0;
const unsigned long ATTENDANCE_WINDOW_START = 0;      // 0 sec after session start
const unsigned long ATTENDANCE_WINDOW_LATE = 300;     // 5 min for "Late" cutoff
const unsigned long ATTENDANCE_WINDOW_CLOSE = 600;    // 10 min absolute cutoff

// ===== HARDWARE OBJECTS =====
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address if needed (0x3F alternative)
byte nuidPICC[4];

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
  lcd.print("TapIn Loading...");
  delay(1000);
  
  // ===== INITIALIZE RFID =====
  SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  // ===== SESSION START =====
  sessionStartTime = millis() / 1000;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TapIn Ready");
  Serial.println("TapIn Ready - Waiting for cards...");
  delay(500);
}
 
// ===== HELPER FUNCTIONS =====

void rejectCard(String reason) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("REJECTED");
  lcd.setCursor(0, 1);
  lcd.print(reason);
  
  Serial.print("REJECT: ");
  Serial.println(reason);
  
  // Red LED + Buzzer (1 long beep)
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

bool scanForBLE() {
  // TODO: BLE verification in Phase 2 (adds too much size)
  // For now, always return true
  // Feature: Scan for student's phone UUID (anti-proxy attendance)
  Serial.println("BLE: Disabled in v1 (Phase 2 feature)");
  return true;
}

void logAttendanceToSupabase(byte* uid, int seat, String status) {
  // TODO: Supabase integration in Phase 2 (adds too much size)
  // For now, just log to Serial
  // Feature: Send attendance to cloud database via REST API
  
  Serial.println("--- ATTENDANCE LOG ---");
  Serial.print("UID: ");
  for (byte i = 0; i < 4; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Seat: ");
  Serial.println(seat);
  Serial.print("Status: ");
  Serial.println(status);
  Serial.println("(Send to Supabase in Phase 2)");
}

void loop() {
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
  
  // ===== PRINT UID TO SERIAL =====
  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  // ===== BLE PROXIMITY VERIFICATION =====
  bool bleVerified = scanForBLE();
  
  if (!bleVerified) {
    rejectCard("Phone Not Found");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  
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
    status = "ABSENT (Closed)";
  }
  
  // ===== DISPLAY SUCCESS ON LCD =====
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Seat: ");
  lcd.print(seat);
  lcd.print(" ");
  lcd.print(status);
  lcd.setCursor(0, 1);
  lcd.print("ID: ");
  for (byte i = 0; i < 2; i++) {
    if (rfid.uid.uidByte[i] < 0x10) lcd.print("0");
    lcd.print(rfid.uid.uidByte[i], HEX);
  }
  
  // ===== SERIAL OUTPUT =====
  Serial.print("Seat: ");
  Serial.println(seat);
  Serial.print("Status: ");
  Serial.println(status);
  
  // ===== SEND TO SUPABASE =====
  logAttendanceToSupabase(rfid.uid.uidByte, seat, status);
  
  // ===== SUCCESS FEEDBACK (Green LED + Buzzer) =====
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
