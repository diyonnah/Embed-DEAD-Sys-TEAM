// ============================================
// TapIn RFID - WORKING VERSION
// ============================================
// Corrected pin configuration + Seat display, LED, Buzzer

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// CORRECTED PINS
#define SS_PIN 5
#define RST_PIN 0       // ← CORRECT PIN (was GPIO 4 - WRONG!)
#define SCK_PIN 14
#define MOSI_PIN 19
#define MISO_PIN 23

// LED and Buzzer pins
#define GREEN_LED 25
#define RED_LED 26
#define BUZZER 27

// LCD I2C (0x27 or 0x3F - adjust if needed)
LiquidCrystal_I2C lcd(0x27, 16, 2);

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Store current card UID
byte currentUID[4];
boolean cardPresent = false;

void setup() { 
  Serial.begin(9600);   // ← CORRECT BAUD RATE (was 115200 - WRONG!)
  
  // Setup pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("TapIn Loading...");
  delay(2000);
  lcd.clear();
  
  // Initialize SPI and RFID
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  
  // Setup RFID key
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  // Initialize display
  lcd.setCursor(0, 0);
  lcd.print("TapIn Ready");
  lcd.setCursor(0, 1);
  lcd.print("Tap your card...");
  
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║          TapIn RFID System             ║"));
  Serial.println(F("║         CORRECTED VERSION             ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("System ready. Waiting for cards..."));
}
 
void loop() {
  // Check if new card present
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Check if card was read
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Get card type
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  
  Serial.print(F("Card Type: "));
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check if MIFARE Classic card
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("✗ Not a MIFARE Classic card"));
    showError("Invalid Card");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // Check if new card
  boolean newCard = false;
  if (rfid.uid.uidByte[0] != currentUID[0] || 
      rfid.uid.uidByte[1] != currentUID[1] || 
      rfid.uid.uidByte[2] != currentUID[2] || 
      rfid.uid.uidByte[3] != currentUID[3]) {
    newCard = true;
    
    // Store new UID
    for (byte i = 0; i < 4; i++) {
      currentUID[i] = rfid.uid.uidByte[i];
    }
  }

  if (newCard) {
    Serial.println(F("✓ New card detected"));
    
    // Print UID
    Serial.print(F("UID (Hex): "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    
    Serial.print(F("UID (Dec): "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    
    // Show success
    showSuccess();
    
    // Simulate seat assignment (you can replace with database lookup)
    int seatNumber = assignSeat(rfid.uid.uidByte);
    displaySeat(seatNumber);
    
  } else {
    Serial.println(F("Card read previously"));
  }

  // Halt PICC
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  delay(500);
}

// ============================================
// HELPER FUNCTIONS
// ============================================

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

// Show success feedback
void showSuccess() {
  digitalWrite(GREEN_LED, HIGH);
  
  // Beep buzzer
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
  
  digitalWrite(GREEN_LED, LOW);
}

// Show error feedback
void showError(const char* message) {
  digitalWrite(RED_LED, HIGH);
  
  // Beep 3 times
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    delay(150);
  }
  
  digitalWrite(RED_LED, LOW);
  
  // Display error on LCD
  lcd.clear();
  lcd.print("Error:");
  lcd.setCursor(0, 1);
  lcd.print(message);
  delay(2000);
}

// Assign seat number (can be replaced with database query)
int assignSeat(byte *uid) {
  // Simple hash-based seat assignment
  // In production, query Supabase with the UID
  int seat = ((uid[0] + uid[1] + uid[2] + uid[3]) % 30) + 1;
  return seat;
}

// Display seat on LCD
void displaySeat(int seatNumber) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Seat: ");
  lcd.print(seatNumber);
  lcd.setCursor(0, 1);
  lcd.print("Welcome!");
  
  Serial.print(F("Assigned Seat: "));
  Serial.println(seatNumber);
  
  delay(3000);
  
  // Return to ready state
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TapIn Ready");
  lcd.setCursor(0, 1);
  lcd.print("Tap your card...");
}
