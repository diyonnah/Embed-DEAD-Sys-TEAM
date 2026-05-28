# RFID Card Setup Guide
## Step-by-Step Instructions for Programming Student ID Cards

---

## **Part 1: Understanding RFID Technology**

### **What is RFID?**
RFID stands for Radio-Frequency Identification. It uses radio waves to identify and track objects or people. In the TapIn system, each student's ID card contains an RFID chip that stores a unique identifier (UID).

### **RFID Components Used in TapIn**
- **MFRC522 Reader Module** - Reads and writes data to RFID cards
- **RFID Cards/Tags** - Contactless cards with embedded microchips
- **UID (Unique Identifier)** - A unique code stored on each card (typically 4 bytes)

### **RFID Card Types**
| Type | Frequency | Read Range | Writable |
|------|-----------|-----------|----------|
| **Mifare Classic 1K** | 13.56 MHz | 0-10 cm | Yes |
| **Mifare Ultra Light** | 13.56 MHz | 0-10 cm | Yes |
| **RFID Keyfobs** | 13.56 MHz | 0-10 cm | Yes |
| **Blank RFID Cards** | 13.56 MHz | 0-10 cm | Yes |

---

## **Part 2: Initial Setup of RFID Reader Module**

### **Step 1: Verify MFRC522 Module Connections**

Before programming cards, ensure the MFRC522 module is properly connected to the ESP32:

| MFRC522 Pin | ESP32 Pin | Purpose |
|-------------|-----------|---------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| RST | GPIO 4 | Reset |
| SDA (CS) | GPIO 5 | Chip Select |
| SCK | GPIO 14 | Clock |
| MOSI | GPIO 19 | Master Out Slave In |
| MISO | GPIO 23 | Master In Slave Out |

### **Step 2: Test MFRC522 Connection**

Create a simple test sketch to verify the module is working:

```cpp
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 4
#define SCK_PIN 14
#define MOSI_PIN 19
#define MISO_PIN 23

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  
  Serial.println("RFID Reader Initialized");
  Serial.println("Waiting for card...");
}

void loop() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("Card UID: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}
```

**Testing Steps:**
1. Upload the sketch to ESP32
2. Open Serial Monitor (115200 baud)
3. Tap a card on the RFID reader
4. Verify the UID appears in Serial Monitor
5. Note the UID format (e.g., "AB CD EF 01")

---

## **Part 3: Reading RFID Card UIDs**

### **Step 3: Scan and Record Student Card UIDs**

Once the MFRC522 module is working, proceed to scan all student cards:

**Process:**
1. Prepare a list or spreadsheet with all student information:
   - Student Number
   - Student Name
   - Date of Scan

2. Run the test sketch from Step 2

3. For each student:
   - Have them tap their ID card on the RFID reader
   - Record the UID from Serial Monitor
   - Add the UID to your student database spreadsheet

**Example Spreadsheet:**
| Student Number | Student Name | RFID UID | BLE UUID | Status |
|---|---|---|---|---|
| 2021-001 | Juan Dela Cruz | AB CD EF 01 | UUID-xxxx | ✓ Recorded |
| 2021-002 | Maria Santos | 12 34 56 78 | UUID-yyyy | ✓ Recorded |
| 2021-003 | Pedro Reyes | 9A 8B 7C 6D | UUID-zzzz | ✓ Recorded |

### **Step 4: Upload UIDs to Supabase Database**

Once all UIDs are collected, upload them to Supabase:

**Manual Method (Dashboard):**
1. Go to Supabase Dashboard
2. Open `students` table
3. Click "Insert Row" for each student
4. Fill in:
   - `student_number` - e.g., "2021-001"
   - `student_name` - Full name
   - `rfid_uid` - UID from card scan (without spaces, lowercase)
   - `ble_uuid` - Leave blank for now (will be generated later)

**Bulk Upload Method (CSV Import):**
1. Prepare CSV file:
   ```csv
   student_number,student_name,rfid_uid
   2021-001,Juan Dela Cruz,abcdef01
   2021-002,Maria Santos,12345678
   2021-003,Pedro Reyes,9a8b7c6d
   ```

2. In Supabase Dashboard:
   - Go to `students` table
   - Click "Import Data" → "Upload CSV"
   - Select your CSV file
   - Map columns:
     - CSV `student_number` → DB `student_number`
     - CSV `student_name` → DB `student_name`
     - CSV `rfid_uid` → DB `rfid_uid`
   - Click "Import"

---

## **Part 4: Writing Data to RFID Cards (Optional)**

### **Step 5: Understand RFID Card Memory Structure**

MFRC522-compatible cards (Mifare Classic 1K) have:
- **1024 bytes** total storage
- **64 blocks** (16 bytes each)
- **First 4 blocks** reserved for UID and system data (read-only)
- **Remaining blocks** available for custom data

### **Step 6: Write Student Number to Card**

Create a sketch to write student data to cards:

```cpp
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 4
#define SCK_PIN 14
#define MOSI_PIN 19
#define MISO_PIN 23

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  
  // Default key for Mifare Classic
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  Serial.println("RFID Card Writer Ready");
  Serial.println("Please enter student number and tap card:");
}

void loop() {
  if (Serial.available() > 0) {
    String studentNumber = Serial.readStringUntil('\n');
    Serial.println("Tap card to write: " + studentNumber);
    
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      // Read card UID
      Serial.print("Card UID: ");
      for (byte i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        Serial.print(rfid.uid.uidByte[i], HEX);
      }
      Serial.println();
      
      // Authenticate and write data
      if (writeStudentNumber(studentNumber)) {
        Serial.println("✓ Data written successfully!");
      } else {
        Serial.println("✗ Write failed");
      }
      
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
}

bool writeStudentNumber(String data) {
  // Use block 4 for writing student data
  byte blockAddr = 4;
  
  // Create data buffer (16 bytes)
  byte buffer[16];
  data.getBytes(buffer, 16);
  
  // Pad with zeros if needed
  for (int i = data.length(); i < 16; i++) {
    buffer[i] = 0x00;
  }
  
  // Authenticate
  MFRC522::StatusCode status = rfid.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    blockAddr,
    &key,
    &(rfid.uid)
  );
  
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Auth failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return false;
  }
  
  // Write data
  status = rfid.MIFARE_Write(blockAddr, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Write failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return false;
  }
  
  return true;
}
```

### **Step 7: Test Card Writing**

1. Upload the sketch
2. Open Serial Monitor (115200 baud)
3. Type student number (e.g., "2021-001")
4. Tap blank RFID card
5. Verify success message

---

## **Part 5: Verifying RFID Card Setup**

### **Step 8: Create Verification Sketch**

Create a comprehensive verification script:

```cpp
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 4
#define SCK_PIN 14
#define MOSI_PIN 19
#define MISO_PIN 23

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  Serial.println("=== RFID Card Verification System ===");
  Serial.println("Tap a card to verify its contents");
}

void loop() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.println("\n--- Card Detected ---");
    
    // Display UID
    Serial.print("UID: ");
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      Serial.print(rfid.uid.uidByte[i], HEX);
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();
    
    // Display card type
    Serial.print("Card Type: ");
    byte piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));
    
    // Read blocks
    readCardBlocks();
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    delay(2000);
  }
}

void readCardBlocks() {
  byte blockAddr = 4;
  byte buffer[18];
  byte size = sizeof(buffer);
  
  // Authenticate
  MFRC522::StatusCode status = rfid.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    blockAddr,
    &key,
    &(rfid.uid)
  );
  
  if (status == MFRC522::STATUS_OK) {
    // Read block
    status = rfid.MIFARE_Read(blockAddr, buffer, &size);
    
    if (status == MFRC522::STATUS_OK) {
      Serial.print("Block 4 Data: ");
      for (byte i = 0; i < 16; i++) {
        if (buffer[i] >= 0x20 && buffer[i] <= 0x7E) {
          Serial.print((char)buffer[i]);
        } else {
          Serial.print(".");
        }
      }
      Serial.println();
    }
  }
}
```

### **Step 9: Verification Checklist**

For each student card, verify:

- [ ] UID is readable and unique
- [ ] UID is stored in Supabase database
- [ ] Card responds quickly (< 100ms)
- [ ] Card can be read from 0-10 cm distance
- [ ] Card data matches student information
- [ ] Card can be read multiple times consistently

---

## **Part 6: Bulk Card Distribution**

### **Step 10: Prepare Cards for Distribution**

**Before Distribution:**
1. Label each card with student number
2. Create a checklist for distribution
3. Prepare instruction sheet for students

**Distribution Process:**
1. Have students sign for their card
2. Provide quick BLE setup instructions
3. Verify each card works with test tap
4. Keep distribution log for records

**Distribution Template:**
| Student Number | Student Name | Card Given | Card Verified | Signature |
|---|---|---|---|---|
| 2021-001 | Juan Dela Cruz | ✓ | ✓ | _______ |
| 2021-002 | Maria Santos | ✓ | ✓ | _______ |
| 2021-003 | Pedro Reyes | ✓ | ✓ | _______ |

### **Step 11: Create Student Instruction Card**

Print and include with each card:

```
═════════════════════════════════════
        TAPIN - RFID Card Instructions
═════════════════════════════════════

Your RFID Card Setup:
• Student Number: 2021-001
• Card UID: AB CD EF 01

How to Use:
1. Before class, visit the TapIn BLE page
2. Enter your student number
3. Activate BLE broadcasting
4. At the classroom door, tap your card
5. Your seat number will display

Important:
✓ Keep your card in good condition
✓ Do not bend or scratch the card
✓ Do not expose to extreme heat
✓ Report lost cards immediately

Questions? Contact: [Your Email]
═════════════════════════════════════
```

---

## **Part 7: Troubleshooting RFID Issues**

### **Step 12: Common Problems & Solutions**

#### **Problem: Card Not Detected**

**Causes & Solutions:**
1. **Loose connections**
   - Recheck SPI wiring (SCK, MOSI, MISO, CS, RST)
   - Test with different card
   - Try different USB power supply

2. **Card not compatible**
   - Verify card is 13.56 MHz RFID
   - Check if card is blank (not already programmed elsewhere)
   - Try known working card for comparison

3. **MFRC522 module defective**
   - Test module SPI communication with oscilloscope
   - Replace module if confirmed defective

**Test Procedure:**
```cpp
void diagnosticTest() {
  Serial.println("Starting MFRC522 Diagnostic...");
  
  if (rfid.PCD_PerformSelfTest()) {
    Serial.println("✓ Self-test passed");
  } else {
    Serial.println("✗ Self-test failed - Module may be defective");
  }
  
  byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print("Firmware version: 0x");
  Serial.println(v, HEX);
}
```

#### **Problem: Inconsistent UID Readings**

**Causes & Solutions:**
1. **Dirty card contacts**
   - Clean card surface with soft cloth
   - Clean RFID reader antenna with compressed air

2. **Card orientation issues**
   - Ensure card is centered on reader
   - Try rotating card slightly
   - Check antenna alignment

3. **Electromagnetic interference**
   - Move away from other RF sources
   - Check for nearby WiFi routers
   - Test in different locations

#### **Problem: UID Changes Between Reads**

**Likely Causes:**
1. **Card UID not fully set** - Reformat and reprogram card
2. **Defective card** - Replace with new card
3. **MFRC522 SPI issue** - Check SPI clock speed (set to ~SPI_CLOCK_DIV8)

```cpp
void setSPISpeed() {
  // In setup():
  SPI.setClockDivider(SPI_CLOCK_DIV8); // Reduce speed for stability
}
```

#### **Problem: Write Operation Fails**

**Causes & Solutions:**
1. **Card may be locked**
   - Some cards come factory locked
   - Try default key: FF FF FF FF FF FF
   - Use specific unlock sketches for branded cards

2. **Incorrect block address**
   - Don't write to blocks 0-3 (reserved for UID)
   - Use blocks 4-62 for custom data
   - Block 3 contains key information

3. **Authentication failure**
   - Verify you're using correct key
   - Reset card if possible
   - Try factory default key first

---

## **Part 8: Advanced RFID Configuration**

### **Step 13: Set Custom RFID Keys (Security)**

For enhanced security, change default RFID keys:

```cpp
// Default key (factory)
MFRC522::MIFARE_Key keyA = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// Custom key (more secure)
MFRC522::MIFARE_Key customKey = {
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55
};

// Change key for a block
bool changeKey(byte blockAddr, MFRC522::MIFARE_Key newKey) {
  // First authenticate with old key
  MFRC522::StatusCode status = rfid.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    blockAddr,
    &keyA,
    &(rfid.uid)
  );
  
  if (status != MFRC522::STATUS_OK) return false;
  
  // Trailer block contains key information
  byte trailerBlock = (blockAddr / 4) * 4 + 3;
  byte buffer[18] = {0};
  
  // Copy new key to buffer
  for (byte i = 0; i < 6; i++) {
    buffer[i] = newKey.keyByte[i];
  }
  
  // Write new key
  status = rfid.MIFARE_Write(trailerBlock, buffer, 18);
  return (status == MFRC522::STATUS_OK);
}
```

### **Step 14: Clone RFID Cards (Backup)**

Create backups of existing cards:

```cpp
// Read entire card
bool backupCard(byte sourceCard[]) {
  byte blockAddr;
  byte buffer[18];
  byte size = sizeof(buffer);
  MFRC522::StatusCode status;
  
  for (blockAddr = 0; blockAddr < 64; blockAddr++) {
    // Authenticate if needed
    if (blockAddr % 4 == 0) {
      status = rfid.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        blockAddr,
        &key,
        &(rfid.uid)
      );
    }
    
    // Read block
    status = rfid.MIFARE_Read(blockAddr, buffer, &size);
    
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Read error at block ");
      Serial.println(blockAddr);
      return false;
    }
    
    // Save to array
    for (byte i = 0; i < 16; i++) {
      sourceCard[blockAddr * 16 + i] = buffer[i];
    }
  }
  
  return true;
}

// Write to new card
bool restoreCard(byte sourceCard[]) {
  byte blockAddr;
  byte buffer[16];
  MFRC522::StatusCode status;
  
  for (blockAddr = 4; blockAddr < 64; blockAddr++) {
    if (blockAddr % 4 == 0) {
      status = rfid.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        blockAddr,
        &key,
        &(rfid.uid)
      );
    }
    
    // Copy from source
    for (byte i = 0; i < 16; i++) {
      buffer[i] = sourceCard[blockAddr * 16 + i];
    }
    
    // Write to target card
    status = rfid.MIFARE_Write(blockAddr, buffer, 16);
    
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Write error at block ");
      Serial.println(blockAddr);
      return false;
    }
  }
  
  return true;
}
```

---

## **Part 9: Quality Assurance Testing**

### **Step 15: Comprehensive QA Checklist**

Before deploying cards to students:

- [ ] **Physical Inspection**
  - Cards have no visible damage
  - Cards are not bent or cracked
  - Labels are clearly printed
  - Cards are properly labeled with student numbers

- [ ] **Functional Testing**
  - Each card's UID is unique
  - UID matches database records
  - Card can be read from 5cm distance
  - Card can be read from 10cm distance
  - Read time is < 200ms
  - Card works consistently (test 10 times)

- [ ] **Data Integrity**
  - Student number written on card matches database
  - Written data survives multiple read cycles
  - No data corruption after 100 reads

- [ ] **Durability Testing**
  - Card works after handling (10 test taps)
  - Card not affected by slight bending
  - Card works at different temperatures
  - Card works after light scratching

### **Step 16: Create QA Test Log**

```
═════════════════════════════════════════════════════
        RFID CARD QUALITY ASSURANCE LOG
═════════════════════════════════════════════════════

Batch Number: BATCH-001
Date: May 28, 2026
Total Cards: 35
Tested By: _______________

Card Number | UID | Read Test | Write Test | Durability | Status
─────────────────────────────────────────────────────────────────
1           | ABCD | ✓ | ✓ | ✓ | PASS
2           | 1234 | ✓ | ✓ | ✓ | PASS
...

Failed Cards: 0 / 35
Pass Rate: 100%
Notes: ___________________________________

Signed: ________________  Date: __________
═════════════════════════════════════════════════════
```

---

## **Part 10: Maintenance & Replacement**

### **Step 17: Card Maintenance Schedule**

**Monthly:**
- [ ] Check for damaged or lost cards
- [ ] Test random sample of cards (5-10)
- [ ] Clean RFID reader with compressed air

**Semester:**
- [ ] Test all active cards
- [ ] Replace worn or damaged cards
- [ ] Update database with new UIDs if replaced

**Year:**
- [ ] Audit all card records
- [ ] Deactivate graduated student cards
- [ ] Recycle old cards

### **Step 18: Replacement Procedure**

If a student loses or damages their card:

1. **Report Loss/Damage**
   - Student submits request to admin

2. **Deactivate Old Card**
   - Mark old UID as inactive in Supabase
   - Search for fraud attempts with old UID

3. **Issue New Card**
   - Scan new blank card
   - Record new UID in database
   - Update student record with new UID
   - Update BLE UUID if needed

4. **Test New Card**
   - Verify new card works
   - Provide to student
   - Update distribution log

---

## **Reference: RFID Card Specifications**

### **Standard RFID Card Specs**
- **Frequency:** 13.56 MHz
- **Type:** ISO/IEC 14443A
- **Memory:** 1KB (Mifare Classic 1K)
- **UID Length:** 4 bytes (32 bits)
- **Read Range:** 0-10 cm
- **Operating Voltage:** 4.5V - 5.5V
- **Temperature Range:** 0°C to 60°C

### **UID Format**
```
Byte 1 | Byte 2 | Byte 3 | Byte 4
─────────────────────────────────
0xAB | 0xCD | 0xEF | 0x01
   (Stored in hexadecimal format)
```

### **Memory Map**
| Blocks | Purpose | Readable | Writable |
|--------|---------|----------|----------|
| 0 | Manufacturer Block | Yes | No |
| 1-2 | UID & Check Digits | Yes | No |
| 3 | System Data | Yes | No |
| 4-62 | **Data Blocks** (User) | Yes | Yes |
| 63 | Trailer (Keys) | Yes | Yes (with key) |

---

## **Troubleshooting Reference**

| Symptom | Possible Cause | Solution |
|---------|---|---|
| No card detected | Connection issue | Check SPI wiring |
| UID changes | Dirty contacts | Clean card & reader |
| Write fails | Card locked | Use factory key or replace card |
| Slow readings | Electromagnetic noise | Move away from interference |
| Card not recognized in DB | UID mismatch | Verify UID format (no spaces, lowercase) |
| Card works intermittently | Bad card | Test with known working card |
| Module won't initialize | Power issue | Check 3.3V supply |

---

## **Quick Reference Commands**

### **Arduino Serial Monitor Commands**
- Send `SCAN` - Start scanning for cards
- Send `STOP` - Stop scanning
- Send `TEST` - Run diagnostic test
- Send `WRITE:2021-001` - Write student number to card

### **Supabase Queries**
```sql
-- Find student by UID
SELECT * FROM students WHERE rfid_uid = 'abcdef01';

-- Update student RFID
UPDATE students SET rfid_uid = 'newuid' WHERE id = 5;

-- Find duplicate UIDs
SELECT rfid_uid, COUNT(*) FROM students GROUP BY rfid_uid HAVING COUNT(*) > 1;
```

---

## **Resources & Links**

- [MFRC522 Documentation](https://www.nxp.com/products/semiconductors/embedded_security_encryption/rfid_identification/)
- [RFID Library GitHub](https://github.com/miguelbalboa/rfid)
- [ISO/IEC 14443A Standard](https://www.iso.org/standard/74649.html)
- [Mifare Classic 1K Datasheet](https://www.nxp.com/docs/en/data-sheet/MF1S703F.pdf)

---

**Document Version:** 1.0  
**Last Updated:** May 28, 2026  
**Created for:** TapIn Project - Computer Engineering Department, PUP
