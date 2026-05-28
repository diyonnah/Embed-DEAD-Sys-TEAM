# TapIn Implementation Guide
## Step-by-Step Instructions to Build the Smart Classroom Device

---

## **Phase 1: Hardware Setup & Assembly**

### **Step 1: Gather All Components**
- [ ] ESP32 Development Board
- [ ] MFRC522 RFID Reader Module
- [ ] 16x2 LCD Display with I2C Module
- [ ] Green LED (with 220Ω resistor)
- [ ] Red LED (with 220Ω resistor)
- [ ] Piezo Buzzer
- [ ] Breadboard
- [ ] Jumper Wires (Male-to-Male, Male-to-Female)
- [ ] USB Power Adapter (5V)
- [ ] RFID Cards/Tags for testing

### **Step 2: Connect MFRC522 RFID Reader to ESP32**
Wire the MFRC522 module using SPI interface:
- MFRC522 VCC → ESP32 3.3V
- MFRC522 GND → ESP32 GND
- MFRC522 RST → ESP32 GPIO 22
- MFRC522 SDA/CS → ESP32 GPIO 5
- MFRC522 SCK → ESP32 GPIO 18
- MFRC522 MOSI → ESP32 GPIO 23
- MFRC522 MISO → ESP32 GPIO 19

### **Step 3: Connect LCD (I2C) to ESP32**
- LCD VCC → ESP32 5V (or 3.3V depending on module)
- LCD GND → ESP32 GND
- LCD SDA → ESP32 GPIO 21
- LCD SCL → ESP32 GPIO 22

### **Step 4: Connect LEDs and Buzzer**
- Green LED Anode → 220Ω Resistor → ESP32 GPIO 25
- Green LED Cathode → ESP32 GND
- Red LED Anode → 220Ω Resistor → ESP32 GPIO 26
- Red LED Cathode → ESP32 GND
- Buzzer Positive → ESP32 GPIO 27
- Buzzer Negative → ESP32 GND

### **Step 5: Power the ESP32**
- Connect USB Power Adapter to ESP32 board
- Verify all connections are secure on the breadboard
- Test that LEDs light up individually (optional manual test)

---

## **Phase 2: Development Environment Setup**

### **Step 6: Install Arduino IDE or PlatformIO**
Choose one development platform:

**Option A: Arduino IDE**
1. Download Arduino IDE from https://www.arduino.cc/en/software
2. Install ESP32 Board Manager:
   - Go to Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Open Boards Manager and search for "ESP32" → Install

**Option B: PlatformIO (Recommended)**
1. Install Visual Studio Code
2. Install PlatformIO extension in VS Code
3. Create new PlatformIO project with board "ESP32 Dev Module"

### **Step 7: Install Required Libraries**
Install the following libraries in your IDE:
- `MFRC522` (by GithubCommunity)
- `LiquidCrystal_I2C` (by Frank de Brabander)
- `WiFi` (built-in for ESP32)
- `ArduinoJson` (by Benoit Blanchon)
- `Supabase` (REST API communication) or use HTTP requests via `HTTPClient`

---

## **Phase 3: Supabase Cloud Database Setup**

### **Step 8: Create Supabase Project**
1. Go to https://supabase.com/ and sign up for free account
2. Create a new project
3. Note the project URL and API keys

### **Step 9: Create Database Tables**
Create the following tables in Supabase PostgreSQL:

**Table 1: students**
```sql
CREATE TABLE students (
  id SERIAL PRIMARY KEY,
  student_number VARCHAR(50) UNIQUE NOT NULL,
  student_name VARCHAR(255) NOT NULL,
  rfid_uid VARCHAR(50) UNIQUE NOT NULL,
  ble_uuid VARCHAR(100),
  created_at TIMESTAMP DEFAULT NOW()
);
```

**Table 2: sessions**
```sql
CREATE TABLE sessions (
  id SERIAL PRIMARY KEY,
  session_name VARCHAR(255) NOT NULL,
  session_type VARCHAR(50), -- 'class' or 'exam'
  start_time TIMESTAMP NOT NULL,
  late_cutoff TIMESTAMP NOT NULL,
  absolute_cutoff TIMESTAMP NOT NULL,
  total_seats INT NOT NULL,
  created_at TIMESTAMP DEFAULT NOW()
);
```

**Table 3: seat_assignments**
```sql
CREATE TABLE seat_assignments (
  id SERIAL PRIMARY KEY,
  session_id INT REFERENCES sessions(id),
  student_id INT REFERENCES students(id),
  seat_number INT NOT NULL,
  UNIQUE(session_id, student_id)
);
```

**Table 4: attendance_logs**
```sql
CREATE TABLE attendance_logs (
  id SERIAL PRIMARY KEY,
  session_id INT REFERENCES sessions(id),
  student_id INT REFERENCES students(id),
  tap_timestamp TIMESTAMP DEFAULT NOW(),
  attendance_status VARCHAR(50), -- 'Present', 'Late', 'Absent'
  ble_verified BOOLEAN DEFAULT FALSE
);
```

**Table 5: fraud_attempts**
```sql
CREATE TABLE fraud_attempts (
  id SERIAL PRIMARY KEY,
  session_id INT REFERENCES sessions(id),
  rfid_uid VARCHAR(50),
  reason VARCHAR(255), -- 'BLE_NOT_DETECTED', 'WRONG_BLE'
  attempt_timestamp TIMESTAMP DEFAULT NOW()
);
```

### **Step 10: Enable Row Level Security (Optional but Recommended)**
- Go to Supabase Dashboard → Authentication → Policies
- Configure policies to restrict database access appropriately

---

## **Phase 4: ESP32 Firmware Development**

### **Step 11: Create Main Arduino Sketch**
Create `tapin_main.ino` with the following sections:

#### **11.1: Include Libraries & Define Pins**
```cpp
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <time.h>

// Pin Definitions
#define SS_PIN 5
#define RST_PIN 22
#define GREEN_LED 25
#define RED_LED 26
#define BUZZER_PIN 27

// I2C LCD Address (typically 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RFID Reader
MFRC522 rfid(SS_PIN, RST_PIN);

// WiFi & Supabase Configuration
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* supabaseUrl = "YOUR_SUPABASE_URL";
const char* supabaseKey = "YOUR_SUPABASE_KEY";
```

#### **11.2: Setup Function**
```cpp
void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("TapIn Loading...");
  delay(2000);
  lcd.clear();
  
  // Initialize SPI & RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Initialize WiFi
  connectToWiFi();
  
  // Set NTP time
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  // Initialize BLE
  BLEDevice::init("");
}
```

#### **11.3: Main Loop Function**
```cpp
void loop() {
  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  // Check for new RFID card
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    handleRFIDTap();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  
  delay(100);
}
```

#### **11.4: RFID Handler Function**
```cpp
void handleRFIDTap() {
  // Get RFID UID
  String rfidUID = getRFIDUID();
  Serial.println("RFID Tap: " + rfidUID);
  
  // Beep on tap
  beep(1, 100);
  lcd.clear();
  lcd.print("Verifying...");
  
  // Query Supabase for student
  int studentId = getStudentByRFID(rfidUID);
  
  if (studentId == -1) {
    // Unknown card
    displayError("Unknown Card");
    beep(3, 150);
    return;
  }
  
  // Get student's registered BLE UUID
  String bleUUID = getBLEUUIDFromStudent(studentId);
  
  // Perform BLE scan
  bool bleVerified = scanForBLE(bleUUID);
  
  if (!bleVerified) {
    // BLE verification failed
    displayError("Phone Not Detected");
    recordFraudAttempt(rfidUID, "BLE_NOT_DETECTED");
    beep(3, 150);
    return;
  }
  
  // Get current session and attendance status
  int sessionId = getCurrentSessionId();
  String attendanceStatus = determineAttendanceStatus(sessionId);
  
  // Get seat number
  int seatNumber = getSeatNumber(studentId, sessionId);
  
  // Log attendance
  logAttendance(sessionId, studentId, attendanceStatus, true);
  
  // Display success
  displaySuccess(seatNumber, attendanceStatus);
  beep(2, 100);
}
```

#### **11.5: BLE Scan Function**
```cpp
bool scanForBLE(String targetUUID) {
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  
  BLEScanResults foundDevices = pBLEScan->start(3); // 3 second scan
  
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    String deviceUUID = device.getAddress().toString().c_str();
    
    if (deviceUUID == targetUUID) {
      return true;
    }
  }
  
  return false;
}
```

#### **11.6: Supabase Query Functions**
```cpp
int getStudentByRFID(String rfidUID) {
  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/students?rfid_uid=eq." + rfidUID;
  
  http.begin(url);
  http.addHeader("apikey", supabaseKey);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    if (doc.size() > 0) {
      return doc[0]["id"];
    }
  }
  
  http.end();
  return -1;
}

String getBLEUUIDFromStudent(int studentId) {
  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/students?id=eq." + String(studentId);
  
  http.begin(url);
  http.addHeader("apikey", supabaseKey);
  
  int httpCode = http.GET();
  String bleUUID = "";
  
  if (httpCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    if (doc.size() > 0) {
      bleUUID = doc[0]["ble_uuid"].as<String>();
    }
  }
  
  http.end();
  return bleUUID;
}

int getCurrentSessionId() {
  // Query for active session based on current time
  HTTPClient http;
  time_t now = time(nullptr);
  String timestamp = String(ctime(&now));
  
  String url = String(supabaseUrl) + "/rest/v1/sessions?start_time=lte." + timestamp + "&absolute_cutoff=gt." + timestamp;
  
  http.begin(url);
  http.addHeader("apikey", supabaseKey);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    if (doc.size() > 0) {
      return doc[0]["id"];
    }
  }
  
  http.end();
  return -1;
}

int getSeatNumber(int studentId, int sessionId) {
  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/seat_assignments?student_id=eq." + String(studentId) + "&session_id=eq." + String(sessionId);
  
  http.begin(url);
  http.addHeader("apikey", supabaseKey);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    
    if (doc.size() > 0) {
      return doc[0]["seat_number"];
    }
  }
  
  http.end();
  return -1;
}
```

#### **11.7: Helper Functions**
```cpp
void connectToWiFi() {
  WiFi.begin(ssid, password);
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    lcd.clear();
    lcd.print("WiFi OK");
    delay(1000);
  } else {
    Serial.println("\nWiFi Failed");
    lcd.clear();
    lcd.print("WiFi Failed");
  }
}

String getRFIDUID() {
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  return uidString;
}

String determineAttendanceStatus(int sessionId) {
  // Query session details and compare with current time
  time_t now = time(nullptr);
  
  // Logic to determine if Present or Late
  // Return "Present" or "Late"
  return "Present";
}

void logAttendance(int sessionId, int studentId, String status, bool bleVerified) {
  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/attendance_logs";
  
  DynamicJsonDocument doc(256);
  doc["session_id"] = sessionId;
  doc["student_id"] = studentId;
  doc["attendance_status"] = status;
  doc["ble_verified"] = bleVerified;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  http.begin(url);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Content-Type", "application/json");
  
  http.POST(jsonString);
  http.end();
}

void recordFraudAttempt(String rfidUID, String reason) {
  HTTPClient http;
  String url = String(supabaseUrl) + "/rest/v1/fraud_attempts";
  
  DynamicJsonDocument doc(256);
  doc["rfid_uid"] = rfidUID;
  doc["reason"] = reason;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  http.begin(url);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Content-Type", "application/json");
  
  http.POST(jsonString);
  http.end();
}

void displaySuccess(int seatNumber, String status) {
  digitalWrite(GREEN_LED, HIGH);
  lcd.clear();
  lcd.print("Seat: ");
  lcd.print(seatNumber);
  lcd.setCursor(0, 1);
  lcd.print(status);
  delay(3000);
  digitalWrite(GREEN_LED, LOW);
  lcd.clear();
}

void displayError(String error) {
  digitalWrite(RED_LED, HIGH);
  lcd.clear();
  lcd.print("Error:");
  lcd.setCursor(0, 1);
  lcd.print(error);
  delay(2000);
  digitalWrite(RED_LED, LOW);
  lcd.clear();
}

void beep(int count, int duration) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}
```

---

## **Phase 5: Web Dashboard Development**

### **Step 12: Create Web Dashboard Project Structure**
```
web-dashboard/
├── index.html
├── css/
│   └── style.css
├── js/
│   ├── main.js
│   └── supabase-client.js
└── pages/
    ├── login.html
    ├── session-config.html
    ├── student-registration.html
    ├── attendance-monitor.html
    └── reports.html
```

### **Step 13: Create HTML Files**

**index.html** - Main Dashboard
```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>TapIn - Classroom Attendance System</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body>
  <div class="container">
    <nav class="navbar">
      <h1>TapIn Dashboard</h1>
      <ul>
        <li><a href="#session-config">Configure Session</a></li>
        <li><a href="#student-registration">Register Students</a></li>
        <li><a href="#attendance-monitor">Monitor Attendance</a></li>
        <li><a href="#reports">Reports</a></li>
      </ul>
    </nav>
    
    <main>
      <section id="session-config" class="section">
        <h2>Session Configuration</h2>
        <form id="sessionForm">
          <label>Session Name:</label>
          <input type="text" id="sessionName" required>
          
          <label>Session Type:</label>
          <select id="sessionType" required>
            <option value="class">Regular Class</option>
            <option value="exam">Examination</option>
          </select>
          
          <label>Start Time:</label>
          <input type="datetime-local" id="startTime" required>
          
          <label>Late Cutoff:</label>
          <input type="datetime-local" id="lateCutoff" required>
          
          <label>Absolute Cutoff:</label>
          <input type="datetime-local" id="absoluteCutoff" required>
          
          <label>Total Seats:</label>
          <input type="number" id="totalSeats" required>
          
          <button type="submit">Create Session</button>
        </form>
      </section>
      
      <section id="student-registration" class="section">
        <h2>Student Registration</h2>
        <form id="studentForm">
          <label>Student Number:</label>
          <input type="text" id="studentNumber" required>
          
          <label>Student Name:</label>
          <input type="text" id="studentName" required>
          
          <label>RFID UID:</label>
          <input type="text" id="rfidUID" placeholder="Tap RFID card" required>
          
          <label>BLE UUID:</label>
          <input type="text" id="bleUUID" required>
          
          <button type="submit">Register Student</button>
        </form>
        
        <h3>Registered Students</h3>
        <table id="studentTable">
          <thead>
            <tr>
              <th>Student Number</th>
              <th>Name</th>
              <th>RFID UID</th>
              <th>BLE UUID</th>
            </tr>
          </thead>
          <tbody id="studentTableBody"></tbody>
        </table>
      </section>
      
      <section id="attendance-monitor" class="section">
        <h2>Real-Time Attendance Monitor</h2>
        <div id="sessionStatus"></div>
        <table id="attendanceTable">
          <thead>
            <tr>
              <th>Student</th>
              <th>Seat</th>
              <th>Tap Time</th>
              <th>Status</th>
              <th>BLE Verified</th>
            </tr>
          </thead>
          <tbody id="attendanceTableBody"></tbody>
        </table>
      </section>
      
      <section id="reports" class="section">
        <h2>Reports & Export</h2>
        <button id="exportExcel">Export to Excel</button>
        <button id="exportPDF">Export to PDF</button>
        <div id="reportContainer"></div>
      </section>
    </main>
  </div>
  
  <script src="https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2"></script>
  <script src="js/supabase-client.js"></script>
  <script src="js/main.js"></script>
</body>
</html>
```

### **Step 14: Create JavaScript Files**

**js/supabase-client.js** - Supabase Client Setup
```javascript
const SUPABASE_URL = 'YOUR_SUPABASE_URL';
const SUPABASE_KEY = 'YOUR_SUPABASE_KEY';

const { createClient } = supabase;
const supabaseClient = createClient(SUPABASE_URL, SUPABASE_KEY);

// Export for use in other files
window.supabaseClient = supabaseClient;
```

**js/main.js** - Dashboard Logic
```javascript
// Session Configuration Handler
document.getElementById('sessionForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  
  const sessionData = {
    session_name: document.getElementById('sessionName').value,
    session_type: document.getElementById('sessionType').value,
    start_time: document.getElementById('startTime').value,
    late_cutoff: document.getElementById('lateCutoff').value,
    absolute_cutoff: document.getElementById('absoluteCutoff').value,
    total_seats: parseInt(document.getElementById('totalSeats').value)
  };
  
  const { data, error } = await supabaseClient
    .from('sessions')
    .insert([sessionData]);
  
  if (error) {
    alert('Error creating session: ' + error.message);
  } else {
    alert('Session created successfully!');
    document.getElementById('sessionForm').reset();
  }
});

// Student Registration Handler
document.getElementById('studentForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  
  const studentData = {
    student_number: document.getElementById('studentNumber').value,
    student_name: document.getElementById('studentName').value,
    rfid_uid: document.getElementById('rfidUID').value,
    ble_uuid: document.getElementById('bleUUID').value
  };
  
  const { data, error } = await supabaseClient
    .from('students')
    .insert([studentData]);
  
  if (error) {
    alert('Error registering student: ' + error.message);
  } else {
    alert('Student registered successfully!');
    document.getElementById('studentForm').reset();
    loadStudents();
  }
});

// Load Students
async function loadStudents() {
  const { data, error } = await supabaseClient
    .from('students')
    .select('*');
  
  if (error) {
    console.error('Error loading students:', error);
    return;
  }
  
  const tbody = document.getElementById('studentTableBody');
  tbody.innerHTML = '';
  
  data.forEach(student => {
    const row = tbody.insertRow();
    row.innerHTML = `
      <td>${student.student_number}</td>
      <td>${student.student_name}</td>
      <td>${student.rfid_uid}</td>
      <td>${student.ble_uuid}</td>
    `;
  });
}

// Real-Time Attendance Monitor
async function monitorAttendance() {
  const { data, error } = await supabaseClient
    .from('attendance_logs')
    .select('*, students(student_name), seat_assignments(seat_number)')
    .order('tap_timestamp', { ascending: false })
    .limit(50);
  
  if (error) {
    console.error('Error loading attendance:', error);
    return;
  }
  
  const tbody = document.getElementById('attendanceTableBody');
  tbody.innerHTML = '';
  
  data.forEach(log => {
    const row = tbody.insertRow();
    row.innerHTML = `
      <td>${log.students.student_name}</td>
      <td>${log.seat_assignments?.seat_number || 'N/A'}</td>
      <td>${new Date(log.tap_timestamp).toLocaleString()}</td>
      <td>${log.attendance_status}</td>
      <td>${log.ble_verified ? '✓' : '✗'}</td>
    `;
  });
}

// Export to Excel
document.getElementById('exportExcel').addEventListener('click', async () => {
  const { data, error } = await supabaseClient
    .from('attendance_logs')
    .select('*, students(student_number, student_name), seat_assignments(seat_number)');
  
  if (error) {
    alert('Error exporting data: ' + error.message);
    return;
  }
  
  // Use a library like xlsx to create Excel file
  // This is a simplified example
  let csvContent = 'Student Number,Student Name,Seat,Tap Time,Status,BLE Verified\n';
  
  data.forEach(log => {
    csvContent += `${log.students.student_number},${log.students.student_name},${log.seat_assignments?.seat_number || 'N/A'},${log.tap_timestamp},${log.attendance_status},${log.ble_verified}\n`;
  });
  
  downloadCSV(csvContent, 'attendance_report.csv');
});

// Download CSV Helper
function downloadCSV(csv, filename) {
  const link = document.createElement('a');
  link.href = 'data:text/csv;charset=utf-8,' + encodeURIComponent(csv);
  link.download = filename;
  link.click();
}

// Initialize on page load
window.addEventListener('load', () => {
  loadStudents();
  monitorAttendance();
  setInterval(monitorAttendance, 5000); // Refresh every 5 seconds
});
```

### **Step 15: Create CSS Styling**

**css/style.css**
```css
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background-color: #f5f5f5;
  color: #333;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
}

.navbar {
  background-color: #2c3e50;
  color: white;
  padding: 1rem 2rem;
  display: flex;
  justify-content: space-between;
  align-items: center;
  box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
}

.navbar h1 {
  font-size: 1.8rem;
}

.navbar ul {
  list-style: none;
  display: flex;
  gap: 2rem;
}

.navbar a {
  color: white;
  text-decoration: none;
  transition: color 0.3s;
}

.navbar a:hover {
  color: #3498db;
}

main {
  padding: 2rem;
}

.section {
  background-color: white;
  border-radius: 8px;
  padding: 2rem;
  margin-bottom: 2rem;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.section h2 {
  margin-bottom: 1.5rem;
  color: #2c3e50;
  border-bottom: 2px solid #3498db;
  padding-bottom: 0.5rem;
}

.section h3 {
  margin-top: 2rem;
  margin-bottom: 1rem;
  color: #34495e;
}

form {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

label {
  font-weight: 600;
  color: #34495e;
}

input, select {
  padding: 0.75rem;
  border: 1px solid #bdc3c7;
  border-radius: 4px;
  font-size: 1rem;
  transition: border-color 0.3s;
}

input:focus, select:focus {
  outline: none;
  border-color: #3498db;
  box-shadow: 0 0 5px rgba(52, 152, 219, 0.3);
}

button {
  padding: 0.75rem 1.5rem;
  background-color: #3498db;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-weight: 600;
  transition: background-color 0.3s;
}

button:hover {
  background-color: #2980b9;
}

button:active {
  transform: scale(0.98);
}

table {
  width: 100%;
  border-collapse: collapse;
  margin-top: 1rem;
}

th, td {
  padding: 0.75rem;
  text-align: left;
  border-bottom: 1px solid #ecf0f1;
}

th {
  background-color: #ecf0f1;
  font-weight: 600;
  color: #2c3e50;
}

tr:hover {
  background-color: #f9f9f9;
}

#sessionStatus {
  padding: 1rem;
  background-color: #ecf0f1;
  border-radius: 4px;
  margin-bottom: 1rem;
}

.status-present {
  color: #27ae60;
  font-weight: 600;
}

.status-late {
  color: #e67e22;
  font-weight: 600;
}

.status-absent {
  color: #e74c3c;
  font-weight: 600;
}
```

---

## **Phase 6: Student BLE Broadcasting Page**

### **Step 16: Create Student BLE Page**

**student-ble.html** - Student Interface
```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>TapIn - Activate BLE Broadcasting</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      margin: 0;
    }
    .container {
      background: white;
      padding: 2rem;
      border-radius: 10px;
      box-shadow: 0 10px 25px rgba(0, 0, 0, 0.2);
      text-align: center;
    }
    h1 {
      color: #333;
    }
    input {
      padding: 0.75rem;
      width: 100%;
      border: 1px solid #ddd;
      border-radius: 4px;
      margin-bottom: 1rem;
      font-size: 1rem;
    }
    button {
      padding: 0.75rem 2rem;
      background-color: #667eea;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 1rem;
    }
    button:hover {
      background-color: #764ba2;
    }
    #status {
      margin-top: 1rem;
      padding: 1rem;
      border-radius: 4px;
      display: none;
    }
    .success {
      background-color: #d4edda;
      color: #155724;
      display: block !important;
    }
    .error {
      background-color: #f8d7da;
      color: #721c24;
      display: block !important;
    }
    #countdown {
      font-size: 2rem;
      font-weight: bold;
      color: #667eea;
      margin-top: 1rem;
      display: none;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>TapIn - Activate BLE</h1>
    <p>Enter your student number to activate BLE broadcasting</p>
    <input type="text" id="studentNumber" placeholder="Student Number">
    <button onclick="activateBLE()">Activate Broadcasting</button>
    <div id="status"></div>
    <div id="countdown"></div>
  </div>
  
  <script src="https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2"></script>
  <script>
    const SUPABASE_URL = 'YOUR_SUPABASE_URL';
    const SUPABASE_KEY = 'YOUR_SUPABASE_KEY';
    
    const { createClient } = supabase;
    const supabaseClient = createClient(SUPABASE_URL, SUPABASE_KEY);
    
    async function activateBLE() {
      const studentNumber = document.getElementById('studentNumber').value;
      const statusDiv = document.getElementById('status');
      
      if (!studentNumber) {
        statusDiv.textContent = 'Please enter student number';
        statusDiv.className = 'error';
        return;
      }
      
      try {
        // Query student
        const { data, error } = await supabaseClient
          .from('students')
          .select('id, ble_uuid')
          .eq('student_number', studentNumber)
          .single();
        
        if (error || !data) {
          statusDiv.textContent = 'Student not found';
          statusDiv.className = 'error';
          return;
        }
        
        let bleUUID = data.ble_uuid;
        
        // If no BLE UUID, generate one
        if (!bleUUID) {
          bleUUID = 'uuid-' + Math.random().toString(36).substr(2, 9);
          
          // Update student record
          await supabaseClient
            .from('students')
            .update({ ble_uuid: bleUUID })
            .eq('id', data.id);
        }
        
        // Store UUID in localStorage
        localStorage.setItem('bleUUID', bleUUID);
        localStorage.setItem('studentNumber', studentNumber);
        
        statusDiv.textContent = `BLE UUID: ${bleUUID}\nBroadcasting for 30 seconds...`;
        statusDiv.className = 'success';
        
        // Start countdown
        startCountdown(30);
        
        // Start BLE broadcasting simulation
        startBLEBroadcast(bleUUID, 30);
        
      } catch (error) {
        statusDiv.textContent = 'Error: ' + error.message;
        statusDiv.className = 'error';
      }
    }
    
    function startCountdown(seconds) {
      const countdownDiv = document.getElementById('countdown');
      countdownDiv.style.display = 'block';
      let remaining = seconds;
      
      const interval = setInterval(() => {
        countdownDiv.textContent = remaining + 's';
        remaining--;
        
        if (remaining < 0) {
          clearInterval(interval);
          countdownDiv.textContent = 'Broadcasting ended';
        }
      }, 1000);
    }
    
    function startBLEBroadcast(uuid, duration) {
      // This would use Web Bluetooth API in real implementation
      // For now, it's a placeholder
      console.log('Broadcasting BLE UUID:', uuid, 'for', duration, 'seconds');
    }
  </script>
</body>
</html>
```

---

## **Phase 7: Testing & Deployment**

### **Step 17: Unit Testing**
- [ ] Test RFID card reading functionality
- [ ] Test BLE scanning and UUID matching
- [ ] Test LED and buzzer feedback
- [ ] Test LCD display output
- [ ] Test WiFi connectivity and reconnection

### **Step 18: Integration Testing**
- [ ] Test complete RFID + BLE flow
- [ ] Test attendance logging to Supabase
- [ ] Test real-time dashboard updates
- [ ] Test session configuration
- [ ] Test fraud attempt logging

### **Step 19: Field Testing**
- [ ] Deploy device in actual classroom
- [ ] Conduct test attendance sessions
- [ ] Verify all students can tap in successfully
- [ ] Check BLE detection at various distances (1-3 meters)
- [ ] Confirm seat assignments display correctly

### **Step 20: Deployment**
- [ ] Flash finalized firmware to ESP32
- [ ] Deploy web dashboard to hosting service (Firebase, Vercel, Netlify, etc.)
- [ ] Configure production Supabase database
- [ ] Set up WiFi credentials in classroom
- [ ] Train faculty on dashboard usage
- [ ] Create user documentation and quick-start guides

---

## **Phase 8: Post-Deployment & Maintenance**

### **Step 21: Monitoring & Support**
- [ ] Monitor attendance logs for errors
- [ ] Track fraud attempt statistics
- [ ] Gather feedback from faculty and students
- [ ] Make necessary bug fixes

### **Step 22: Future Enhancements**
- [ ] Add email notifications for high fraud attempt rates
- [ ] Implement QR code scanning as alternative to RFID
- [ ] Add analytics dashboard for attendance trends
- [ ] Develop mobile app for student check-in
- [ ] Support for multiple rooms/departments

---

## **Troubleshooting Guide**

### **ESP32 Issues**
| Problem | Solution |
|---------|----------|
| ESP32 not uploading code | Check USB driver, try different USB cable, verify board selection in IDE |
| WiFi connection fails | Verify SSID/password, check WiFi signal strength, restart ESP32 |
| RFID reader not working | Check SPI wiring, verify RST/CS pins, test with known working card |
| BLE scanning not finding devices | Ensure BLE is enabled on smartphones, increase scan duration, check proximity |

### **Database Issues**
| Problem | Solution |
|---------|----------|
| Cannot connect to Supabase | Verify API keys, check WiFi connection, check Supabase project status |
| Data not syncing | Check API request format, verify table structure, check Supabase policies |
| Slow queries | Add database indexes, optimize API requests, check network bandwidth |

### **Dashboard Issues**
| Problem | Solution |
|---------|----------|
| Dashboard not updating | Refresh browser, check Supabase connection, verify API keys |
| Export not working | Check browser console for errors, verify table data exists |
| Session not appearing | Verify session times are correct, check date/time on ESP32 |

---

## **Bill of Materials (BOM) Summary**

| Component | Qty | Est. Cost |
|-----------|-----|-----------|
| ESP32 Dev Board | 1 | $10-15 |
| MFRC522 RFID Module | 1 | $5-8 |
| 16x2 LCD + I2C Module | 1 | $5-8 |
| LEDs (Green & Red) | 2 | $1 |
| Piezo Buzzer | 1 | $1-2 |
| Resistors, Wires, Breadboard | 1 set | $5 |
| USB Power Adapter | 1 | $5-10 |
| RFID Cards | 30+ | $10-15 |
| **TOTAL** | | **~$42-59** |

---

## **References & Resources**

- [ESP32 Documentation](https://www.espressif.com/)
- [MFRC522 Library](https://github.com/miguelbalboa/rfid)
- [Supabase Documentation](https://supabase.com/docs)
- [Arduino Web Editor](https://create.arduino.cc/editor)
- [Web Bluetooth API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API)

---

**Document Version:** 1.0  
**Last Updated:** May 28, 2026  
**Author:** TapIn Development Team
