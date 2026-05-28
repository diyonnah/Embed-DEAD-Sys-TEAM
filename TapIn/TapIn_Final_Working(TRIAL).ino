// ============================================
// TapIn RFID - WORKING BASE + LED/BUZZER/SEAT
// ============================================
// Based on the code that actually works!

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 0

// LED and Buzzer pins
#define GREEN_LED 25
#define RED_LED 26
#define BUZZER 27
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

void setup() { 
  Serial.begin(9600);
  
  // Setup LED and Buzzer pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  // Startup beep
  digitalWrite(GREEN_LED, HIGH);
  delay(200);
  digitalWrite(GREEN_LED, LOW);
  
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║        TapIn RFID System               ║"));
  Serial.println(F("║   Scanning MIFARE Classic NUID         ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.print(F("Using key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println(F(""));
  Serial.println(F("Ready. Tap your card..."));
}
 
void loop() {

  // Reset the loop if no new card present on the sensor/reader
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.println(F(""));
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    showError();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("✓ A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    
    // SUCCESS FEEDBACK
    showSuccess();
    
    // ASSIGN SEAT based on UID
    int seatNumber = assignSeat(rfid.uid.uidByte, rfid.uid.size);
    Serial.print(F(""));
    Serial.print(F("═════════════════════════════════════════"));
    Serial.println(F(""));
    Serial.print(F("Assigned Seat: "));
    Serial.println(seatNumber);
    Serial.print(F("═════════════════════════════════════════"));
    Serial.println(F(""));
    
  }
  else {
    Serial.println(F("Card read previously."));
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}


/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

/**
 * Show success feedback - Green LED + 2 beeps
 */
void showSuccess() {
  digitalWrite(GREEN_LED, HIGH);
  
  // 2 short beeps
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
  
  digitalWrite(GREEN_LED, LOW);
}

/**
 * Show error feedback - Red LED + 3 beeps
 */
void showError() {
  digitalWrite(RED_LED, HIGH);
  
  // 3 beeps
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    delay(150);
  }
  
  digitalWrite(RED_LED, LOW);
}

/**
 * Assign seat number based on UID
 * Simple algorithm: sum of UID bytes modulo 30
 * In production: query Supabase database
 */
int assignSeat(byte *uid, byte size) {
  byte sum = 0;
  for (byte i = 0; i < size; i++) {
    sum += uid[i];
  }
  // Seats 1-30
  int seat = (sum % 30) + 1;
  return seat;
}
