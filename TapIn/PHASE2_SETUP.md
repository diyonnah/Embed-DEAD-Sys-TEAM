# TapIn Phase 2 Setup Guide: Supabase + Web Dashboard

## **Overview**
This guide helps you set up cloud database integration and the web dashboard for real-time attendance monitoring.

---

## **Step 1: Supabase Database Setup**

### **1.1 Create Tables in Supabase**

Go to **Supabase Dashboard** → **SQL Editor** and run these queries:

#### **Create `sessions` table:**
```sql
CREATE TABLE IF NOT EXISTS sessions (
  id TEXT PRIMARY KEY,
  class_name TEXT NOT NULL,
  instructor_name TEXT,
  start_time TIMESTAMP,
  end_time TIMESTAMP,
  total_seats INT DEFAULT 30,
  status TEXT DEFAULT 'scheduled'
);
```

#### **Create `students` table:**
```sql
CREATE TABLE IF NOT EXISTS students (
  id BIGSERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  student_number TEXT,
  card_uid TEXT UNIQUE NOT NULL,
  seat_number INT,
  ble_uuid TEXT UNIQUE,
  created_at TIMESTAMP DEFAULT now()
);
```

#### **Create `attendance_logs` table:**
```sql
CREATE TABLE IF NOT EXISTS attendance_logs (
  id BIGSERIAL PRIMARY KEY,
  session_id TEXT NOT NULL,
  card_uid TEXT NOT NULL,
  seat_number INT,
  attendance_status TEXT NOT NULL,
  timestamp BIGINT,
  created_at TIMESTAMP DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_session_id ON attendance_logs(session_id);
CREATE INDEX IF NOT EXISTS idx_card_uid ON attendance_logs(card_uid);
```

#### **Disable RLS (for development only):**
```sql
ALTER TABLE sessions DISABLE ROW LEVEL SECURITY;
ALTER TABLE students DISABLE ROW LEVEL SECURITY;
ALTER TABLE attendance_logs DISABLE ROW LEVEL SECURITY;
```

### **1.2 Enable Row Level Security (RLS)**

For each table, go to **Authentication** → **Policies** and create a policy:

**For public read/write (development only):**
```
- Policy: Enable all (FOR ALL, TO public)
- SQL: CREATE POLICY "Enable all" ON table_name FOR ALL TO authenticated, anon USING (true) WITH CHECK (true);
```

⚠️ **For production:** Use more restrictive policies!

---

## **Step 2: Arduino Setup**

### **2.1 Install Required Libraries**

In Arduino IDE, go to **Sketch → Include Library → Manage Libraries** and install:
- `WiFi` (built-in)
- `HTTPClient` (built-in)
- `ArduinoJson` by Benoit Blanchon
- `LiquidCrystal_I2C` by Frank de Brabander

### **2.2 Upload the Sketch**

1. Open `TapIn_With_Supabase.ino`
2. **IMPORTANT:** Update these lines with your credentials:
   ```cpp
   const char* WIFI_SSID = "YOUR_SSID";
   const char* WIFI_PASSWORD = "YOUR_PASSWORD";
   const char* SUPABASE_URL = "https://YOUR_PROJECT.supabase.co";
   const char* SUPABASE_KEY = "YOUR_ANON_KEY";
   ```

3. Get your credentials from Supabase:
   - **URL:** Project Settings → General → API URL
   - **Anon Key:** Project Settings → API → `anon` key

4. Verify board: **Tools → Board → ESP32 Dev Module**
5. Verify port: **Tools → Port → COM# (ESP32)**
6. Click **Upload**
7. Open **Serial Monitor** (115200 baud) to watch logs

---

## **Step 3: Web Dashboard Setup**

### **3.1 Open Dashboard**

1. Locate `dashboard.html` in your project folder
2. Double-click to open in web browser (or right-click → Open With)
3. You should see the TapIn dashboard

### **3.2 Using the Dashboard**

**Controls:**
- ▶ **Start Session** - Begins attendance session
- ⏹ **Stop Session** - Ends session
- 📥 **Export CSV** - Downloads attendance as spreadsheet
- 🔄 **Refresh** - Manually sync data

**Live Display:**
- **Top Left:** Attendance summary (Present/Late/Absent counts)
- **Top Right:** Live feed of taps (shows card UID, seat, status)
- **Bottom:** Attendance table with all records

---

## **Step 4: Test Everything**

### **4.1 On Hardware:**
1. Open Serial Monitor (115200 baud)
2. You should see: `"TapIn Ready - Tap card to begin"`
3. **Tap an RFID card**
4. Expected output:
   ```
   >>> Card Detected!
   UID: AB CD EF 01
   Seat: 15
   Status: PRESENT
   ✓ Response Code: 201
   Sending to Supabase...
   ```

### **4.2 On Dashboard:**
1. Click **▶ Start Session**
2. You should see:
   - Session ID appears
   - "Session started" in live feed
   - Status badge turns GREEN

### **4.3 Tap Card Again:**
1. On hardware: Card is detected, sent to Supabase
2. On dashboard: Should immediately see record in table + live feed

---

## **Step 5: Verify Supabase Storage**

1. Go to **Supabase Dashboard → SQL Editor**
2. Run:
   ```sql
   SELECT * FROM attendance_logs ORDER BY created_at DESC LIMIT 10;
   ```
3. You should see your attendance records with timestamps

---

## **Troubleshooting**

### **Problem: "✗ Error" in Serial Monitor**
**Solution:**
- Check WiFi is connected: `IP: 192.168.x.x` should appear
- Verify Supabase credentials are correct
- Check SUPABASE_URL has no trailing slash

### **Problem: Dashboard shows no records**
**Solution:**
- Open browser console (F12 → Console tab)
- Check for CORS errors (Supabase might block requests)
- Verify Supabase RLS policies allow read/write

### **Problem: LCD doesn't display**
**Solution:**
- Verify I2C address (0x27 or 0x3F) - try both in code
- Check SDA (GPIO 21) and SCL (GPIO 22) connections
- Run I2C scanner sketch to find address

### **Problem: Card not detected**
**Solution:**
- Verify RST pin is GPIO 0
- Check MFRC522 power (should have LED lit)
- See diagnostic tools in main folder

---

## **What's Working Now (Phase 2)**

✅ RFID card detection (Phase 1)  
✅ LCD display (Phase 1)  
✅ LED/Buzzer feedback (Phase 1)  
✅ **WiFi connectivity** (NEW)  
✅ **Supabase database sync** (NEW)  
✅ **Web dashboard** (NEW)  
✅ **Live attendance viewing** (NEW)  
✅ **CSV export** (NEW)  

---

## **Future Enhancements (Phase 3+)**

📋 **BLE Proximity Verification** - Prevent proxy attendance (requires extra board space)  
📋 **Real-time WebSocket updates** - Live dashboard without manual refresh  
📋 **Student Registration Web Page** - For pre-session enrollment  
📋 **Multi-Room Support** - Multiple TapIn devices per session  
📋 **Fraud Detection** - Alerts for suspicious patterns  

---

## **Next Steps**

1. ✅ Test Phase 2 (this document)
2. 📝 Customize student database with real data
3. 🚀 Deploy to actual classroom
4. 📊 Collect attendance data
5. 🔄 Iterate based on feedback

---

**Questions?** Check the main README or TapIn_Updated_Proposal.md for full project details.
