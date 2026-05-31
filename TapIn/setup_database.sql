-- ============================================
-- TapIn Database Schema - Supabase Setup
-- Run this in Supabase SQL Editor
-- ============================================

-- Drop existing tables (WARNING: loses all data!)
-- DROP TABLE IF EXISTS attendance_logs;
-- DROP TABLE IF EXISTS students;
-- DROP TABLE IF EXISTS sessions;
-- DROP TABLE IF EXISTS sections;

-- ===== CREATE SECTIONS TABLE =====
CREATE TABLE IF NOT EXISTS sections (
  id BIGSERIAL PRIMARY KEY,
  name TEXT UNIQUE NOT NULL,
  created_at TIMESTAMP DEFAULT now()
);

-- ===== CREATE STUDENTS TABLE =====
CREATE TABLE IF NOT EXISTS students (
  id BIGSERIAL PRIMARY KEY,
  section_name TEXT,
  name TEXT NOT NULL,
  student_number TEXT UNIQUE NOT NULL,
  seat_number INT,
  card_uid TEXT UNIQUE NOT NULL,
  ble_id TEXT UNIQUE,
  created_at TIMESTAMP DEFAULT now()
);

-- ===== CREATE SESSIONS TABLE =====
CREATE TABLE IF NOT EXISTS sessions (
  id TEXT PRIMARY KEY,
  class_name TEXT NOT NULL,
  session_type TEXT DEFAULT 'class',
  section_name TEXT,
  instructor_name TEXT,
  total_seats INT DEFAULT 30,
  start_time TIMESTAMP,
  end_time TIMESTAMP,
  status TEXT DEFAULT 'scheduled',
  created_at TIMESTAMP DEFAULT now()
);

-- ===== CREATE ATTENDANCE_LOGS TABLE =====
CREATE TABLE IF NOT EXISTS attendance_logs (
  id BIGSERIAL PRIMARY KEY,
  session_id TEXT REFERENCES sessions(id) ON DELETE CASCADE,
  student_id BIGINT REFERENCES students(id) ON DELETE SET NULL,
  student_name TEXT,
  seat_number INT,
  card_uid TEXT,
  ble_id TEXT,
  attendance_status TEXT NOT NULL,
  timestamp BIGINT,
  created_at TIMESTAMP DEFAULT now()
);

-- ===== CREATE INDEXES =====
CREATE INDEX IF NOT EXISTS idx_students_card_uid ON students(card_uid);
CREATE INDEX IF NOT EXISTS idx_attendance_session ON attendance_logs(session_id);
CREATE INDEX IF NOT EXISTS idx_attendance_card ON attendance_logs(card_uid);

-- ===== DISABLE RLS (for development only) =====
-- Remove RLS restriction so dashboard can read/write
ALTER TABLE sections DISABLE ROW LEVEL SECURITY;
ALTER TABLE students DISABLE ROW LEVEL SECURITY;
ALTER TABLE sessions DISABLE ROW LEVEL SECURITY;
ALTER TABLE attendance_logs DISABLE ROW LEVEL SECURITY;

-- ===== TEST DATA (optional - remove after testing) =====
-- INSERT INTO sections (name) VALUES ('BSCpE 4-1');
-- INSERT INTO students (section_name, name, student_number, seat_number, card_uid, ble_id)
-- VALUES ('BSCpE 4-1', 'Dela Cruz, Juan A.', '2023-001234', 1, '7301A138', 'A1B2C3D4-E5F6-7890-ABCD-1234567890AB');
-- INSERT INTO sessions (id, class_name, session_type, section_name, instructor_name, total_seats, status)
-- VALUES ('session_test_001', 'CMPE 409 - Test', 'class', 'BSCpE 4-1', 'Engr. Rufo Marasigan Jr.', 30, 'scheduled');

-- ===== VERIFICATION =====
-- Check tables exist
-- SELECT table_name FROM information_schema.tables WHERE table_schema = 'public';
-- SELECT * FROM sections;
-- SELECT * FROM students;
-- SELECT * FROM sessions;
-- SELECT * FROM attendance_logs;
