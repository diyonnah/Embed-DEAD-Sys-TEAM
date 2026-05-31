# TapIn System Status

## Database
- Schema updated for sections, students, sessions, attendance_logs.
- student_number, card_uid, and ble_id are unique.
- RLS is disabled for development.

## Web
- Dashboard uses Supabase REST API and expects sessions + attendance_logs.
- Student BLE ID flow is handled on the website (name + card ID -> ble_id).

## Hardware (ESP32)
- BLE verification uses ble_id from Supabase.
- Active session is pulled from Supabase (status = active).
- Attendance window is based on session start and end time.
- Attendance logs include session_id, card_uid, ble_id, seat_number, status, and epoch timestamp.

## Alignment checklist
- Column names are consistent with the schema (ble_id, card_uid, class_name).
- Session time source is from the active session record.
- RFID + BLE two-factor verification is enforced.

## Notes
- Web-to-Supabase connectivity cannot be verified here. Once the SQL is run in Supabase and the dashboard uses the correct URL and key, it should be ready. Verify by creating a session and checking rows in sessions and attendance_logs.
