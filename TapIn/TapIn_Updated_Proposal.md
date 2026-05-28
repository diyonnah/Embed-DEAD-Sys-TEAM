## **Polytechnic University of the Philippines** 

College of Engineering **Computer Engineering Department** 

**CMPE 409** EMBEDDED SYSTEMS 

## **TapIn:** 

**A Smart Classroom Device for Real-Time Attendance Monitoring and Seat Assignment** 

A Project Proposal 

Submitted by: 

**Group 2** 

**Bithao, Daniel D. Esparrago, Jonnah E. Francisco, Vanessa M. Lorzano, Lorenzo Jaimes C. Madriaga, Bryle R. Naval, Jan Francis H. Recote, Jean Terry L.** 

Course/Section: 

**BSCpE 4-1** 

Submitted to: 

**Engr. Rufo I. Marasigan Jr., DEng-CpE, PCpE** 

Date Submitted: **15 May 2026** 

## **I. INTRODUCTION** 

Managing student attendance and seating arrangements in academic institutions has long relied on manual, paper-based processes that are time-consuming, error-prone, and inefficient. In traditional classroom settings, faculty members prepare attendance files at the start of each semester, call students by name one by one, and manually record who is present, late, or absent — a process that requires the instructor's full attention and time that could otherwise be spent on teaching (Al Hajri et al., 2019). This is no different in the Computer Engineering department of the Polytechnic University of the Philippines (PUP), where attendance is currently taken through printed sheets that students sign upon entering the classroom. 

Beyond attendance, examination management introduces another layer of administrative burden. Facilitators must manually count the total number of registered students, select appropriate rooms, allocate students accordingly, and prepare individual seating lists for each exam (Iro et al., 2025). In PUP, this is further reflected during departmental examinations, where students must check a printed seating chart, taped on the door of the examination room, just to locate their assigned seat and confirm the subject of the exam before they can even enter. 

These developments highlight the growing need for an integrated solution that addresses both challenges at once. Rather than treating attendance monitoring and seat assignment as separate administrative tasks, a single automated device can handle both simultaneously, recording whether a student is present, late, or absent the moment they arrive, while instantly informing them of their designated seat number. This eliminates the inefficiencies that departments like the Computer Engineering department of PUP continue to face today, and represents a practical step toward smarter, more efficient classroom and examination management. 

A critical concern in any automated attendance system is the possibility of proxy attendance — a common form of academic dishonesty wherein one student taps in on behalf of another. To address this, TapIn incorporates a two-factor proximity validation mechanism combining RFID identification with Bluetooth Low Energy (BLE) proximity detection, ensuring that the student physically present at the device is indeed the one being recorded. 

## **II. PROJECT OVERVIEW** 

## **Project Description** 

TapIn is an educational device developed using embedded systems technology that makes it easy for students to take attendance and be assigned seats through just one operation. This system is designed for the Department of Computer Engineering at Polytechnic University of the Philippines. 

This application utilizes RFID technology, where students tap their ID cards as they enter the room. If the ID tap is successful and the student's phone BLE signal is detected within proximity, it registers the student's attendance status as "Present" or "Late," depending on the scheduled class time, and also displays the student's seat number during that particular session. Those who fail to tap their ID cards are automatically marked "Absent" when the designated attendance period ends. 

To prevent proxy attendance, TapIn employs a two-factor verification system. In addition to tapping an RFID-embedded ID card, the student's smartphone must be physically present near the device, broadcasting a unique Bluetooth Low Energy (BLE) UUID registered to that specific student. A tap is rejected if no matching BLE signal is detected, effectively eliminating the possibility of one student tapping in on behalf of another. 

All attendance records, student information, and session data are stored in Supabase, a cloud-based PostgreSQL database, which the ESP32 microcontroller communicates with in realtime via WiFi. Faculty members and administrators can manage the system entirely through a webbased dashboard accessible via any browser; no physical input device is required on the hardware unit itself. Through this dashboard, the faculty can register students and their corresponding RFID card IDs and BLE UUIDs, configure session parameters such as total seat count and attendance time window, and monitor attendance status in real time. 

The TapIn system integrates attendance and seat assignment into a simple task for students upon entering the room. 

## **Objectives:** 

1. Automatically track the presence of each student in the class based on his/her entry using an RFID tap and classify each student as either being present, late, or absent based on a predefined time window. 

2. Prevent proxy attendance by requiring both a valid RFID tap and the physical presence of the student's smartphone broadcasting its registered BLE UUID within 1–3 meters of the device. 

3. Assign and display each student's designated seat number on the LCD screen simultaneously upon a successful RFID tap and BLE verification. 

4. Combine attendance tracking and seat assignment into a single tap operation, eliminating the need for separate manual processes at the start of each class or examination. 

5. Provide a web-based dashboard that allows faculty and administrators to register students, configure session parameters, including total number of seats, attendance time window, 

and session type, and monitor attendance in real time without requiring any physical input on the device itself. 

6. Store all attendance logs, student records, and session data in a cloud-based database 

(Supabase) and make them accessible anytime through the web dashboard, eliminating the need for printed sign-in sheets or manual data entry. 

## **Scope** 

TapIn is conceived as an embedded system prototyping platform targeted for deployment with the students of Section 4-1 from the Computer Engineering department at the Polytechnic University of the Philippines. The system shall carry out two primary tasks — attendance tracking and seat allocation — via one single RFID operation combined with BLE proximity verification upon entering the classroom. 

## **III. SYSTEM DESIGN** 

## **Materials** 

The following hardware and software requirements are necessary for the development of 

TapIn: A Smart Classroom Device for Real-Time Attendance Monitoring and Seat Assignment. 

||||
|---|---|---|
|**COMPONENT**|**DESCRIPTION**|**QTY**|
||||
||||
|ESP32<br>Development<br>Board|Main microcontroller for processing, WiFi, and BLE<br>scanning|1|
||||
||||
|MFRC522 RFID Reader<br>Module|Reads student RFID cards via SPI interface|1|



|**COMPONENT**|**DESCRIPTION**|**QTY**|
|---|---|---|
||||
||||
|RFID Cards/Tags|For testing and student ID simulation|—|
||||
||||
|16x2<br>LCD<br>with<br>I2C<br>Module|Displays seat number and attendance status|1|
||||
||||
|Green LED|Visual feedback for successful tap|1|
||||
||||
|Red LED|Visual feedback for rejected/invalid tap|1|
||||
||||
|Buzzer|Audio feedback on card tap|1|
||||
|Resistors (220Ω)|Current limiting for LEDs|2|
||||
||||
|Breadboard|Prototyping and component connections|1|
||||
||||
|Jumper Wires (M-M, M-F)|Electrical connections between components|1 set|
||||
||||
|USB Power Adapter (5V)|Power supply and USB interface for programming the<br>ESP32 board|1|
||||
||||
|Supabase (Free Tier)|Cloud database for storing attendance logs, student<br>records, BLE UUIDs, and session data|—|
||||



## **System Functions** 

## **1. RFID-based Student Identification** 

Upon entry into the classroom, the student's RFID-embedded ID card is read by the MFRC522 module. Each registered card corresponds to a specific student record stored in the Supabase database. Unregistered or unknown cards will not be accepted and will trigger a visual and audio rejection feedback via the red LED and buzzer. 

## **2. BLE Proximity Verification (Anti-Proxy Attendance)** 

After a successful RFID read, the ESP32 activates a BLE scan lasting 1–3 seconds to detect nearby Bluetooth signals. Each registered student has a unique BLE UUID stored in the Supabase 

database, broadcast by their smartphone via the student-facing web. A tap is only accepted when: 

- The scanned RFID belongs to Student A, AND 

- Student A's registered BLE UUID is detected within 1–3 meters. 

If no matching BLE signal is detected, or if a different student's BLE UUID is detected instead, the tap is rejected. The LCD displays "Phone Not Detected" and the red LED is activated. 

This mechanism directly prevents the common proxy attendance scenario where a student submits another student's ID card on their behalf. 

## **3. Automatic Attendance Logging** 

The system automatically records the student's attendance status based on the time of the RFID tap relative to the session's configured time window: 

- Present — tap falls within the allowable time period 

- Late — tap occurs after the allowable time period has expired but before the cutoff 

- Absent — no tap is recorded by the end of the attendance period 

Each log is timestamped using network time synchronized via NTP (Network Time 

Protocol) and immediately synced to the Supabase cloud database via WiFi. 

## **4. Automatic Seat Allocation** 

Upon a successful RFID tap and BLE verification, the system retrieves and displays the student's pre-assigned seat number on the LCD screen. Seat assignments are configured prior to the session through the web dashboard based on the room capacity and enrolled student list. 

## **5. Web-Based Session Configuration and Monitoring** 

All administrative tasks are handled through a web-based dashboard accessible via any browser on any device connected to the internet. Through this dashboard, the faculty or administrator can: 

- Register students and link their RFID card UIDs and BLE UUIDs to their respective records 

- Set the total room seating capacity and assign seat numbers per student 

- Configure the attendance time window (start time, late cutoff, and absolute cutoff) before 

each session 

- Select the session type (regular class or examination) 

- Monitor attendance status in real time as students tap in 

- View and export the complete attendance log after each session (Excel or PDF) 

## **6. Student BLE Broadcasting** 

Students access a dedicated web page before class and enter their student number to activate BLE broadcasting. The page checks Supabase for the student's existing BLE UUID. If one already exists, it is restored from the database — no new UUID is generated. If the student is using the system for the first time, a new UUID is generated, saved to Supabase, and stored in localStorage. Broadcasting runs for 30 seconds. This design ensures that BLE UUIDs remain consistent even if the student clears their browser cache or switches devices. 

## **7. Cloud-Based Attendance Log Storage** 

All attendance records are stored in Supabase and are accessible anytime through the web dashboard. The stored data includes each student's name, attendance status, and exact timestamp per session, eliminating the need for printed forms or manual record-keeping. 

## **Block Diagram** 

The system utilizes an ESP32 as the main microcontroller with wireless connectivity to the database and local web browser through Wi-Fi. The MFRC522 serves as the primary input device for scanning RFID cards. After each RFID read, the ESP32 performs a BLE scan to verify the student's physical presence before logging attendance. For every successful and verified log, the ESP32 activates the green LED, sounds the piezo buzzer, and displays the assigned seat number on the 16×2 LCD. If BLE verification fails or an invalid RFID card is detected, the red LED is activated and the tap is rejected. Attendance records are reflected in the web browser dashboard, while all collected data are stored in the Supabase database. 

## **IV. CONCLUSION** 

## **Summary** 

TapIn is an embedded systems-based device designed to automate and integrate two of the most repetitive administrative tasks in academic settings — attendance monitoring and seat assignment. By leveraging RFID technology, Bluetooth Low Energy (BLE) proximity verification, cloud database storage via Supabase, and a web-based management dashboard, the system enables students to complete both processes through a single ID tap upon entering the classroom. The ESP32 microcontroller serves as the core of the hardware, handling RFID reading, BLE scanning, network time synchronization via NTP, LCD display output, and WiFi-based 

communication with the Supabase cloud database. Faculty and administrators manage all configurations and monitor attendance entirely through the web dashboard, with no need for printed sheets, manual data entry, or physical input on the device. 

The addition of BLE proximity verification addresses the critical problem of proxy attendance, ensuring that the student who taps the RFID card is the same student who is physically present in the room. This two-factor validation — RFID for identity, BLE for physical presence — makes TapIn a secure and trustworthy attendance system suitable for both regular class sessions and high-stakes departmental examinations. 

## **Expected Outcomes** 

By the end of the project, the following outcomes are expected: 

- **A.** A fully functional TapIn prototype that accurately reads student RFID cards, verifies BLE proximity, and records attendance status in real time. 

- **B.** Correct and simultaneous seat number assignment displayed on the LCD upon every successful and verified tap. 

- **C.** A working web dashboard that allows faculty to configure sessions, register students, assign seats, and monitor attendance live — including a fraud attempt counter. 

- **D.** A reliable cloud database (Supabase) integration that stores all session and attendance data, 

   - including BLE UUIDs and fraud attempt logs, and makes them retrievable at any time. 

- **E.** Demonstrated resistance to proxy attendance attempts through two-factor RFID + BLE verification. 

