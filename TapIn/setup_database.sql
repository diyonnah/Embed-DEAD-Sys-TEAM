-- ============================================
-- TapIn Database Schema - Supabase Setup
-- Run this in Supabase SQL Editor
-- ============================================

-- Drop existing tables (WARNING: loses all data!)
-- DROP TABLE IF EXISTS attendance_logs;
-- DROP TABLE IF EXISTS students;
-- DROP TABLE IF EXISTS sessions;

-- ===== CREATE SESSIONS TABLE =====
CREATE TABLE IF NOT EXISTS sessions (
  id TEXT PRIMARY KEY,
  class_name TEXT NOT NULL,
  instructor_name TEXT,
  start_time TIMESTAMP,
  end_time TIMESTAMP,
  total_seats INT DEFAULT 30,
  status TEXT DEFAULT 'scheduled'
);

-- ===== CREATE STUDENTS TABLE =====
CREATE TABLE IF NOT EXISTS students (
  id BIGSERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  student_number TEXT,
  card_uid TEXT UNIQUE NOT NULL,
  seat_number INT,
  ble_uuid TEXT UNIQUE,
  created_at TIMESTAMP DEFAULT now()
);

-- ===== CREATE ATTENDANCE_LOGS TABLE =====
CREATE TABLE IF NOT EXISTS attendance_logs (
  id BIGSERIAL PRIMARY KEY,
  session_id TEXT NOT NULL,
  card_uid TEXT NOT NULL,
  seat_number INT,
  attendance_status TEXT NOT NULL,
  timestamp BIGINT,
  created_at TIMESTAMP DEFAULT now()
);

-- ===== CREATE INDEXES =====
CREATE INDEX IF NOT EXISTS idx_attendance_session ON attendance_logs(session_id);
CREATE INDEX IF NOT EXISTS idx_attendance_card ON attendance_logs(card_uid);
CREATE INDEX IF NOT EXISTS idx_students_card ON students(card_uid);

-- ===== DISABLE RLS (for development only) =====
-- Remove RLS restriction so dashboard can read/write
ALTER TABLE sessions DISABLE ROW LEVEL SECURITY;
ALTER TABLE students DISABLE ROW LEVEL SECURITY;
ALTER TABLE attendance_logs DISABLE ROW LEVEL SECURITY;

-- ===== TEST DATA (optional - remove after testing) =====
-- INSERT INTO sessions (id, class_name, instructor_name, total_seats, status)
-- VALUES ('session_test_001', 'CMPE 409 - Test', 'Engr. Rufo Marasigan Jr.', 30, 'scheduled');

-- INSERT INTO students (name, student_number, card_uid, seat_number)
-- VALUES ('Esparrago, Jonnah E.', '2023-001234', '7301A138', 1);

-- ===== VERIFICATION =====
-- Check tables exist
-- SELECT table_name FROM information_schema.tables WHERE table_schema = 'public';

-- Check data
-- SELECT * FROM sessions;
-- SELECT * FROM students;
-- SELECT * FROM attendance_logs;
