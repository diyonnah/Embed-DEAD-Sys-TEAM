# TapIn Flow (Faculty + Student + Hardware)

## High-level overview
TapIn uses a two-factor check for attendance:
1) RFID card UID must be registered.
2) Matching BLE ID from the same student must be detected near the device.

Attendance logs are written to Supabase and shown on the dashboard.

## Student flow (BLE ID generation)
1) Student opens the web page.
2) Selects Student role.
3) Enters name and card ID (RFID UID).
4) Website checks Supabase for a student record with this card UID.
   - If found and ble_id exists: use that ble_id.
   - If found but ble_id is empty: generate a new ble_id and update Supabase.
   - If not found: student must consult the faculty to register the card.
5) Website stores the ble_id in localStorage on the phone.
6) Student starts BLE broadcasting for the BLE ID (10-15 seconds).

## Faculty flow (dashboard)
1) Faculty opens the web page.
2) Selects Faculty role.
3) Logs in or signs up.
4) Creates sections and adds students (manual or Excel import).
   - BLE ID is left blank unless the student generates it from the student portal.
   - Sections are saved by name; students and sessions store section_name in Supabase.
   - Student save enforces section_name sync to Supabase.
   - Deleting a section or student in the UI also deletes the record in Supabase.
5) Creates sessions (class or exam) with section, instructor, time window, and seat count.
6) Activates a session. Dashboard shows live attendance.
7) Ends session; logs remain in History and can be exported.

## Web hosting options
- Standalone: open the dashboard in a browser.
- ESP32-hosted: use the embedded dashboard served from the device (TapIn_Integrated.ino).

## Hardware flow (ESP32 + RFID + BLE)
1) ESP32 boots and connects to WiFi.
2) ESP32 initializes RFID, LCD, and BLE scanner.
3) When a card is tapped:
   - Read card UID.
   - Fetch student record from Supabase by card UID.
   - If no record or missing ble_id: reject.
4) ESP32 scans BLE for a short window (ex: 3 seconds).
   - If expected ble_id is not found: reject.
   - If found: accept.
5) Attendance status is computed using the active session start and end time.
6) ESP32 logs attendance to Supabase with session_id, card_uid, ble_id, seat_number, time, and status.
7) LCD shows seat number and status; buzzer + LED confirm success.

## Session lifecycle
1) Faculty creates and activates a session in the dashboard.
2) ESP32 uses the active session for all attendance logs.
3) When the session ends, faculty deactivates it.
4) Logs move to History view and can be exported.

## Data consistency rules
- `card_uid` must be unique per student.
- `student_number` must be unique per student.
- `ble_id` must be unique per student.
- The ESP32 and dashboard must use the same column names.
