#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// Wi-Fi settings
// Replace these with your actual Wi-Fi credentials.
const char* WIFI_SSID = "happy new year";
const char* WIFI_PASSWORD = "20262026";

// If Wi-Fi credentials are not set or connection fails, the ESP32 starts its own hotspot.
const char* AP_SSID = "ESP32-DHT11-Monitor";
const char* AP_PASSWORD = "12345678";

// Button pins
const int BUTTON_DEC_PIN = 35;
const int BUTTON_INC_PIN = 34;

// LED pins
const int INDICATOR_PIN = 23;
const int LED1_PIN = 2;
const int LED2_PIN = 16;
const int LED3_PIN = 5;
const int LED4_PIN = 19;

// DHT11 pin
const int DHT_PIN = 13;
const int DHT_TYPE = DHT11;

// Control thresholds
const float FAN_ON_TEMP_THRESHOLD = 29.4;

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

const int MIN_LEVEL = 1;
const int MAX_LEVEL = 4;
int level = 1;

bool indicatorState = false;

// Buttons on GPIO34/GPIO35 need external resistors.
// The code auto-detects the idle level at startup, so it can work with
// either pull-up or pull-down wiring as long as the buttons are not pressed
// while the board is booting.
const bool BUTTON_RELEASED = LOW;

unsigned long lastTempReadTime = 0;
const unsigned long TEMP_READ_INTERVAL = 2000;

const unsigned long DEBOUNCE_DELAY = 80;
const unsigned long BUTTON_LOCKOUT_DELAY = 250;

bool lastDecReading = BUTTON_RELEASED;
bool lastIncReading = BUTTON_RELEASED;
bool stableDecState = BUTTON_RELEASED;
bool stableIncState = BUTTON_RELEASED;
bool decIdleState = BUTTON_RELEASED;
bool incIdleState = BUTTON_RELEASED;
bool decPressHandled = false;
bool incPressHandled = false;

unsigned long lastButtonDecChangeTime = 0;
unsigned long lastButtonIncChangeTime = 0;
unsigned long lastAcceptedButtonTime = 0;

float lastTemperatureC = NAN;
float lastHumidity = NAN;
bool hasValidSensorData = false;

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>DHT11 Temperature Tracker</title>
	<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.2/dist/chart.umd.min.js"></script>

	<style>
		@import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap');
		@import url('https://fonts.googleapis.com/css2?family=Gemunu+Libre:wght@700&display=swap');
		@import url('https://fonts.googleapis.com/css2?family=Gayathri:wght@700&display=swap');
		@import url('https://fonts.googleapis.com/css2?family=Bruno+Ace&display=swap');

		:root {
			--bg:          #04080f;
			--border:      rgba(255,255,255,0.08);
			--text-1:      #f1f5f9;
			--text-2:      #94a3b8;
			--text-3:      #4b6080;
			--accent:      #3b82f6;
			--green:       #22c55e;
			--green-dim:   rgba(34,197,94,0.13);
			--red:         #ef4444;
			--red-dim:     rgba(239,68,68,0.13);
			--amber:       #f59e0b;
			--amber-dim:   rgba(245,158,11,0.15);
			--cyan:        #06b6d4;
			--radius:      14px;
			--glass-blur:  blur(20px) saturate(160%);
			--card-bg:     rgba(255,255,255,0.045);
			--card-border: rgba(255,255,255,0.085);
			--card-shadow: 0 2px 24px rgba(0,0,0,0.4), inset 0 1px 0 rgba(255,255,255,0.05);
			--pad:         18px;
		}

		*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

		body {
			font-family: 'Inter', 'Segoe UI', sans-serif;
			text-align: center;
			background: var(--bg);
			background-image:
				radial-gradient(ellipse 120% 55% at 50% 0%, rgba(6,182,212,0.3) 0%, transparent 70%),
				radial-gradient(ellipse 60% 30% at 80% 0%, rgba(168,85,247,0.15) 0%, transparent 60%);
			background-attachment: fixed;
			color: var(--text-1);
			min-height: 100vh;
			display: flex;
			flex-direction: column;
		}

		.app-shell {
			max-width: 900px;
			margin: 0 auto;
			width: 100%;
			flex: 1;
			display: flex;
			flex-direction: column;
			padding-bottom: 40px;
		}
		.app-shell .page-section {
			flex: 1;
			display: flex;
			flex-direction: column;
		}
		.app-shell .dashboard-grid {
			flex: 1;
		}

		.app-header {
			background: rgb(15, 22, 31);
			border-bottom: 1px solid var(--border);
			padding: calc(24px + env(safe-area-inset-top)) 0 24px 40px;
			position: relative;
			overflow: hidden;
		}
		.app-header::before {
			content: '';
			position: absolute;
			width: 317px;
			height: 304px;
			background: radial-gradient(circle, rgba(34, 197, 94, 0.25) 0%, transparent 70%);
			top: -100px;
			left: -50px;
			pointer-events: none;
		}
		.app-header::after {
			content: '';
			position: absolute;
			width: 754px;
			height: 494px;
			background: radial-gradient(circle, rgba(0, 183, 255, 0.2) 0%, transparent 70%);
			top: -200px;
			right: -300px;
			pointer-events: none;
		}
		.header-content {
			position: relative;
			z-index: 1;
			display: flex;
			flex-direction: column;
			text-align: left;
		}
		.group-badge-top {
			font-family: 'Bruno Ace', sans-serif;
			font-size: 22px;
			font-weight: 400;
			color: rgb(0, 183, 255);
			letter-spacing: 0.5px;
			text-shadow: 0 0 12px rgba(0, 183, 255, 0.5), 0 0 30px rgba(0, 183, 255, 0.2);
			margin-bottom: 2px;
		}
		.app-title {
			font-family: 'Gemunu Libre', sans-serif;
			font-size: 32px;
			font-weight: 700;
			color: rgb(255, 255, 255);
			margin: 0;
			letter-spacing: -1px;
			text-align: left;
		}
		.app-subtitle {
			font-family: 'Gayathri', sans-serif;
			font-size: 14px;
			font-weight: 700;
			color: rgb(255, 255, 255);
			line-height: 1.5;
			opacity: 0.9;
			white-space: nowrap;
			margin-top: 4px;
			text-align: left;
		}
		@keyframes pulse {
			0%, 100% { opacity: 1; }
			50%       { opacity: 0.4; }
		}
		@media (max-width: 768px) {
			.app-header {
				padding: calc(20px + env(safe-area-inset-top)) 0 20px 24px;
			}
			.app-title {
				font-size: 24px;
			}
			.app-subtitle {
				font-size: 12px;
			}
			.group-badge-top {
				font-size: 16px;
			}
		}

		@media (orientation: portrait) {
			.app-shell {
				max-width: 100%;
			}
			.page-section {
				padding: 24px 20px 32px;
			}
			.dashboard-grid {
				gap: 14px;
			}
			.dashboard-col {
				gap: 14px;
			}
			.control-card {
				padding: 22px;
			}
			.metric-value {
				font-size: 52px;
			}
			.fan-card {
				padding: 22px;
			}
			.stat-card {
				padding: 16px 12px 14px;
			}
			.stat-value {
				font-size: 22px;
			}
		}

		.page-section {
			padding: 20px 16px 28px;
		}

		.dashboard-grid {
			display: grid;
			grid-template-columns: 1fr 1fr;
			grid-template-rows: 1fr 1fr auto;
			gap: 10px;
			margin: 0 2px 10px;
			min-height: 32vh;
		}
		.dashboard-grid .control-card,
		.dashboard-grid .fan-card,
		.dashboard-grid .chart-card,
		.dashboard-grid .stats-row {
			margin: 0;
		}
		.dashboard-grid .temp-card  { grid-column: 1; grid-row: 1; }
		.dashboard-grid .fan-card   { grid-column: 2; grid-row: 1; }
		.dashboard-grid .hum-card   { grid-column: 1; grid-row: 2; display: flex; flex-direction: column; }
		.dashboard-grid .hum-card .metric-display { flex: 1; display: flex; flex-direction: column; justify-content: center; }
		.dashboard-grid .chart-card { grid-column: 2; grid-row: 2 / 4; display: flex; flex-direction: column; }
		.dashboard-grid .chart-card .chart-wrap { flex: 1; min-height: 0; }
		.dashboard-grid .stats-row  { grid-column: 1; grid-row: 3; }

		.control-card,
		.chart-card,
		.fan-card,
		.stat-card {
			background: var(--card-bg);
			backdrop-filter: var(--glass-blur);
			-webkit-backdrop-filter: var(--glass-blur);
			border: 1px solid var(--card-border);
			border-radius: var(--radius);
			box-shadow: var(--card-shadow);
		}

		.control-card {
			padding: var(--pad);
			width: 100%;
			text-align: left;
			display: flex;
			flex-direction: column;
		}
		.card-header {
			display: flex;
			align-items: center;
			justify-content: space-between;
			margin-bottom: 12px;
		}
		.card-title {
			display: flex;
			align-items: center;
			gap: 8px;
			font-size: 12px;
			font-weight: 700;
			color: var(--text-2);
			text-transform: uppercase;
			letter-spacing: 0.8px;
		}
		.card-icon { font-size: 14px; line-height: 1; }

		.badge {
			display: inline-block;
			padding: 2px 8px;
			border-radius: 20px;
			font-size: 9px;
			font-weight: 700;
			letter-spacing: 0.8px;
			text-transform: uppercase;
			transition: background 0.3s, color 0.3s;
		}
		.badge-normal  { background: var(--green-dim);  color: var(--green);  border: 1px solid rgba(34,197,94,0.25); }
		.badge-warning { background: var(--amber-dim);  color: var(--amber);  border: 1px solid rgba(245,158,11,0.25); }
		.badge-danger  { background: var(--red-dim);    color: var(--red);    border: 1px solid rgba(239,68,68,0.25); }
		.badge-idle    { background: rgba(75,96,128,0.08); color: var(--text-3); border: 1px solid rgba(75,96,128,0.15); }

		.metric-display {
			text-align: center;
			padding: 8px 0 4px;
			flex: 1;
			display: flex;
			flex-direction: column;
			justify-content: center;
		}
		.metric-value {
			font-size: 58px;
			font-weight: 800;
			line-height: 1;
			letter-spacing: -4px;
			font-variant-numeric: tabular-nums;
			transition: color 0.4s, text-shadow 0.4s;
		}
		.metric-unit {
			font-size: 16px;
			font-weight: 600;
			letter-spacing: -0.5px;
			margin-left: 1px;
			opacity: 0.6;
		}
		.metric-label {
			font-size: 9px;
			font-weight: 600;
			color: var(--text-3);
			text-transform: uppercase;
			letter-spacing: 1.5px;
			margin-top: 5px;
		}

		.temp-val         { color: var(--cyan);   text-shadow: 0 0 40px rgba(6,182,212,0.45); }
		.temp-val.warm    { color: var(--amber);  text-shadow: 0 0 40px rgba(245,158,11,0.45); }
		.temp-val.hot     { color: var(--red);    text-shadow: 0 0 40px rgba(239,68,68,0.45); }
		.hum-val          { color: var(--accent); text-shadow: 0 0 40px rgba(59,130,246,0.45); }

		.stats-row {
			display: flex;
			gap: 10px;
			margin: 0 0 10px;
		}
		.stat-card {
			flex: 1;
			padding: 14px 10px 12px;
			text-align: center;
			display: flex;
			flex-direction: column;
			justify-content: center;
		}
		.stat-value {
			font-size: 20px;
			font-weight: 800;
			letter-spacing: -1px;
			font-variant-numeric: tabular-nums;
		}
		.stat-label {
			font-size: 9px;
			font-weight: 600;
			color: var(--text-3);
			text-transform: uppercase;
			letter-spacing: 1.2px;
			margin-top: 4px;
		}
		.stat-card.min .stat-value { color: var(--accent); }
		.stat-card.max .stat-value { color: var(--red); }
		.stat-card.avg .stat-value { color: var(--amber); }

		.refresh-row {
			display: flex;
			align-items: center;
			justify-content: center;
			gap: 6px;
			padding-top: 10px;
			font-size: 9px;
			font-weight: 600;
			color: var(--text-3);
			letter-spacing: 0.5px;
		}
		.refresh-dot {
			width: 5px; height: 5px;
			border-radius: 50%;
			background: var(--green);
			animation: pulse 2s infinite;
		}

		.sim-badge {
			display: inline-flex;
			align-items: center;
			gap: 5px;
			background: rgba(245,158,11,0.15);
			border: 1px solid rgba(245,158,11,0.35);
			border-radius: 20px;
			padding: 3px 9px;
			font-size: 9px;
			font-weight: 700;
			color: var(--amber);
			letter-spacing: 0.8px;
			text-transform: uppercase;
		}
		.sim-badge::before {
			content: '';
			width: 6px; height: 6px;
			border-radius: 50%;
			background: var(--amber);
			animation: pulse 1.4s infinite;
		}

		.chart-card {
			padding: var(--pad);
			width: 100%;
		}
		.chart-card-header {
			display: flex;
			align-items: center;
			justify-content: space-between;
			margin-bottom: 12px;
		}
		.chart-card-title {
			font-size: 12px;
			font-weight: 700;
			color: var(--text-2);
			text-transform: uppercase;
			letter-spacing: 0.8px;
		}
		.chart-legend {
			display: flex;
			gap: 12px;
		}
		.chart-legend-item {
			display: flex;
			align-items: center;
			gap: 5px;
			font-size: 9px;
			font-weight: 600;
			color: var(--text-3);
			letter-spacing: 0.4px;
		}
		.chart-legend-dot {
			width: 7px; height: 7px;
			border-radius: 50%;
		}
		.chart-legend-dot.temp { background: #06b6d4; }
		.chart-legend-dot.hum  { background: #3b82f6; }
		.chart-wrap {
			position: relative;
			height: 200px;
			flex: 1;
			min-height: 0;
		}

		.fan-card {
			padding: var(--pad);
			width: 100%;
			display: flex;
			flex-direction: column;
			gap: 14px;
			transition: border-color 0.4s, box-shadow 0.4s;
		}
		.fan-card.fan-on {
			border-color: rgba(34,197,94,0.35);
			box-shadow: 0 4px 32px rgba(34,197,94,0.12), inset 0 1px 0 rgba(255,255,255,0.06);
		}
		.fan-status-row {
			display: flex;
			justify-content: flex-end;
		}
		.fan-center {
			flex: 1;
			display: flex;
			flex-direction: column;
			align-items: center;
			justify-content: center;
			gap: 6px;
		}
		.fan-label {
			font-size: 13px;
			font-weight: 700;
			color: var(--text-2);
			text-transform: uppercase;
			letter-spacing: 1.2px;
			margin-top: 4px;
		}
		.fan-icon-wrap {
			width: 72px; height: 72px;
			border-radius: 50%;
			display: flex;
			align-items: center;
			justify-content: center;
			background: rgba(255,255,255,0.05);
			border: 1px solid rgba(255,255,255,0.1);
			transition: background 0.4s, border-color 0.4s, box-shadow 0.4s;
			flex-shrink: 0;
		}
		.fan-card.fan-on .fan-icon-wrap {
			background: rgba(34,197,94,0.12);
			border-color: rgba(34,197,94,0.5);
			box-shadow: 0 0 18px rgba(34,197,94,0.4), 0 0 6px rgba(34,197,94,0.6);
		}
		.fan-icon-svg {
			width: 36px; height: 36px;
			stroke: var(--text-3);
			transition: stroke 0.4s, filter 0.4s;
		}
		.fan-card.fan-on .fan-icon-svg {
			stroke: var(--green);
			filter: drop-shadow(0 0 6px rgba(34,197,94,0.9));
		}
		.fan-info-sub {
			font-size: 11px;
			color: var(--text-3);
			margin-top: 3px;
			font-weight: 500;
		}
		.fan-status-pill {
			display: inline-flex;
			align-items: center;
			gap: 6px;
			padding: 6px 14px;
			border-radius: 24px;
			font-size: 11px;
			font-weight: 800;
			letter-spacing: 0.5px;
			text-transform: uppercase;
			transition: background 0.4s, color 0.4s, border-color 0.4s, box-shadow 0.4s;
		}
		.fan-status-pill.on {
			background: var(--green-dim);
			color: var(--green);
			border: 1px solid rgba(34,197,94,0.35);
			box-shadow: 0 0 16px rgba(34,197,94,0.15);
		}
		.fan-status-pill.off {
			background: rgba(75,94,122,0.1);
			color: var(--text-3);
			border: 1px solid rgba(75,94,122,0.2);
			box-shadow: none;
		}
		.fan-status-pill::before {
			content: '';
			width: 8px; height: 8px;
			border-radius: 50%;
			background: currentColor;
			opacity: 0.85;
		}
		.fan-status-pill.on::before { animation: pulse 1.6s infinite; }
		.fan-bottom-row {
			display: flex;
			align-items: center;
			justify-content: space-between;
			padding-top: 12px;
			border-top: 1px solid rgba(255,255,255,0.06);
		}
		.intensity-label {
			font-size: 10px;
			font-weight: 700;
			color: var(--text-3);
			text-transform: uppercase;
			letter-spacing: 1.2px;
		}
		.fan-intensity-ctrl {
			display: flex;
			align-items: center;
			gap: 10px;
		}
		.intensity-btn {
			width: 30px; height: 30px;
			border-radius: 8px;
			border: 1px solid rgba(255,255,255,0.1);
			background: rgba(255,255,255,0.06);
			color: var(--text-2);
			font-size: 18px;
			font-weight: 700;
			line-height: 1;
			cursor: pointer;
			display: flex;
			align-items: center;
			justify-content: center;
			transition: background 0.2s, border-color 0.2s, transform 0.1s;
			font-family: inherit;
		}
		.intensity-btn:hover  { background: rgba(255,255,255,0.12); border-color: rgba(255,255,255,0.22); }
		.intensity-btn:active { transform: scale(0.9); }
		.intensity-btn:disabled { opacity: 0.25; cursor: not-allowed; transform: none; }
		.intensity-display {
			display: flex;
			align-items: center;
			gap: 8px;
		}
		.intensity-bars {
			display: flex;
			align-items: flex-end;
			gap: 3px;
			height: 20px;
		}
		.intensity-bar {
			width: 5px;
			border-radius: 2px;
			background: rgba(255,255,255,0.1);
			transition: background 0.35s;
		}
		.intensity-bar.active { background: var(--text-3); }
		.fan-card.fan-on .intensity-bar.active { background: var(--green); box-shadow: 0 0 6px rgba(34,197,94,0.5); }
		.intensity-num {
			font-size: 18px;
			font-weight: 800;
			color: var(--text-1);
			font-variant-numeric: tabular-nums;
			min-width: 16px;
			text-align: center;
		}

		.app-footer {
			padding: 14px 20px calc(14px + env(safe-area-inset-bottom));
			font-size: 9px;
			font-weight: 600;
			color: var(--text-3);
			background: rgba(6,12,30,0.6);
			backdrop-filter: blur(20px) saturate(160%);
			-webkit-backdrop-filter: blur(20px) saturate(160%);
			border-top: 1px solid var(--border);
			letter-spacing: 1px;
			text-transform: uppercase;
			position: fixed;
			bottom: 0;
			left: 0;
			right: 0;
			z-index: 100;
		}
	</style>
</head>
<body>
	<div class="app-header">
		<div class="header-content">
			<div class="group-badge-top">Group 2</div>
			<div class="app-title">LABORATORY 2</div>
			<div class="app-subtitle">Real-time temperature &amp; humidity monitoring with DHT11 over Wi-Fi</div>
		</div>
	</div>

	<div class="app-shell">
		<div class="page-section">
			<div class="dashboard-grid">
				<div class="control-card temp-card">
					<div class="card-header">
						<div class="card-title"><span class="card-icon"></span>Temperature</div>
						<span class="badge badge-idle" id="tempBadge">—</span>
					</div>
					<div class="metric-display">
						<div class="metric-value temp-val" id="tempVal">--<span class="metric-unit">°C</span></div>
						<div class="metric-label">Celsius</div>
					</div>
				</div>

				<div class="fan-card" id="fanCard">
					<div class="fan-status-row">
						<div class="fan-status-pill off" id="fanPill">OFF</div>
					</div>
					<div class="fan-center">
						<div class="fan-icon-wrap">
							<svg class="fan-icon-svg" viewBox="0 0 24 24" fill="none" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
								<path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path>
								<line x1="12" y1="2" x2="12" y2="12"></line>
							</svg>
						</div>
						<div class="fan-label">Fan</div>
						<div class="fan-info-sub" id="fanSub">Waiting for data…</div>
					</div>
					<div class="fan-bottom-row">
						<span class="intensity-label">Intensity</span>
						<div class="fan-intensity-ctrl">
							<button class="intensity-btn" id="intMinus" onclick="changeFanIntensity(-1)">-</button>
							<div class="intensity-display">
								<div class="intensity-bars">
									<div class="intensity-bar" style="height:6px" data-level="1"></div>
									<div class="intensity-bar" style="height:10px" data-level="2"></div>
									<div class="intensity-bar" style="height:14px" data-level="3"></div>
									<div class="intensity-bar" style="height:20px" data-level="4"></div>
								</div>
								<span class="intensity-num" id="intensityNum">1</span>
							</div>
							<button class="intensity-btn" id="intPlus" onclick="changeFanIntensity(1)">+</button>
						</div>
					</div>
				</div>

				<div class="control-card hum-card">
					<div class="card-header">
						<div class="card-title"><span class="card-icon"></span>Humidity</div>
						<span class="badge badge-idle" id="humBadge">—</span>
					</div>
					<div class="metric-display">
						<div class="metric-value hum-val" id="humVal">--<span class="metric-unit">%</span></div>
						<div class="metric-label">Relative Humidity</div>
					</div>
				</div>

				<div class="chart-card">
					<div class="chart-card-header">
						<div class="chart-card-title">Live Chart</div>
						<div style="display:flex;align-items:center;gap:10px;">
							<span class="sim-badge" id="simBadge">Simulation</span>
							<div class="chart-legend">
								<span class="chart-legend-item"><span class="chart-legend-dot temp"></span>Temp °C</span>
								<span class="chart-legend-item"><span class="chart-legend-dot hum"></span>Humidity %</span>
							</div>
						</div>
					</div>
					<div class="chart-wrap">
						<canvas id="sensorChart"></canvas>
					</div>
				</div>

				<div class="stats-row">
					<div class="stat-card min">
						<div class="stat-value" id="minTemp">--</div>
						<div class="stat-label">Min C</div>
					</div>
					<div class="stat-card avg">
						<div class="stat-value" id="avgTemp">--</div>
						<div class="stat-label">Avg C</div>
					</div>
					<div class="stat-card max">
						<div class="stat-value" id="maxTemp">--</div>
						<div class="stat-label">Max C</div>
					</div>
				</div>
			</div>

			<div class="refresh-row">
				<span class="refresh-dot"></span>
				<span id="lastUpdated">Waiting for data…</span>
			</div>
		</div>
	</div>

	<div class="app-footer">ESP32 &nbsp;&bull;&nbsp; DHT11 Sensor &nbsp;&bull;&nbsp; Polling every 2s</div>

	<script>
		let readings = [];
		const MAX_LOG = 50;
		const CHART_MAX = 30;
		const chartLabels = [];
		const chartTempData = [];
		const chartHumData = [];

		const sensorChart = new Chart(document.getElementById('sensorChart'), {
			type: 'line',
			data: {
				labels: chartLabels,
				datasets: [
					{
						label: 'Temperature (C)',
						data: chartTempData,
						borderColor: '#06b6d4',
						backgroundColor: 'rgba(6,182,212,0.08)',
						borderWidth: 2,
						pointRadius: 2,
						pointHoverRadius: 5,
						tension: 0.4,
						fill: true,
						yAxisID: 'yTemp',
					},
					{
						label: 'Humidity (%)',
						data: chartHumData,
						borderColor: '#3b82f6',
						backgroundColor: 'rgba(59,130,246,0.08)',
						borderWidth: 2,
						pointRadius: 2,
						pointHoverRadius: 5,
						tension: 0.4,
						fill: true,
						yAxisID: 'yHum',
					}
				]
			},
			options: {
				responsive: true,
				maintainAspectRatio: false,
				animation: { duration: 400 },
				interaction: { mode: 'index', intersect: false },
				plugins: {
					legend: { display: false },
					tooltip: {
						backgroundColor: 'rgba(10,20,45,0.92)',
						titleColor: '#94a3b8',
						bodyColor: '#f1f5f9',
						borderColor: 'rgba(255,255,255,0.1)',
						borderWidth: 1,
						padding: 10,
						titleFont: { size: 10, weight: '600' },
						bodyFont: { size: 12, weight: '700' },
					}
				},
				scales: {
					x: {
						ticks: { color: '#4b6080', font: { size: 9 }, maxTicksLimit: 6 },
						grid: { color: 'rgba(255,255,255,0.04)' },
						border: { color: 'rgba(255,255,255,0.06)' },
					},
					yTemp: {
						position: 'left',
						ticks: { color: '#06b6d4', font: { size: 9 } },
						grid: { color: 'rgba(255,255,255,0.04)' },
						border: { color: 'rgba(255,255,255,0.06)' },
						title: { display: true, text: 'C', color: '#06b6d4', font: { size: 9 } },
					},
					yHum: {
						position: 'right',
						ticks: { color: '#3b82f6', font: { size: 9 } },
						grid: { drawOnChartArea: false },
						border: { color: 'rgba(255,255,255,0.06)' },
						title: { display: true, text: '%', color: '#3b82f6', font: { size: 9 } },
					}
				}
			}
		});

		function pushChart(t, h) {
			const label = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
			chartLabels.push(label);
			chartTempData.push(parseFloat(t));
			chartHumData.push(parseFloat(h));
			if (chartLabels.length > CHART_MAX) {
				chartLabels.shift();
				chartTempData.shift();
				chartHumData.shift();
			}
			sensorChart.update();
		}

		function tempClass(t) {
			if (t >= 37) return 'hot';
			if (t >= 30) return 'warm';
			return '';
		}

		function tempBadgeState(t) {
			if (t >= 37) return ['Hot', 'badge-danger'];
			if (t >= 30) return ['Warm', 'badge-warning'];
			return ['Normal', 'badge-normal'];
		}

		function humBadgeState(h) {
			if (h < 30 || h > 70) return ['Dry/Humid', 'badge-warning'];
			return ['Optimal', 'badge-normal'];
		}

		let fanIntensity = 1;

		function renderIntensityBars(level) {
			const fanOn = document.getElementById('fanCard').classList.contains('fan-on');
			document.querySelectorAll('.intensity-bar').forEach(bar => {
				bar.classList.toggle('active', parseInt(bar.dataset.level, 10) <= level);
			});
			const numEl = document.getElementById('intensityNum');
			numEl.textContent = level;
			numEl.style.opacity = fanOn ? '1' : '0.25';
			document.getElementById('intMinus').disabled = (!fanOn || level <= 1);
			document.getElementById('intPlus').disabled = (!fanOn || level >= 4);
		}

		function changeFanIntensity(delta) {
			fanIntensity = Math.min(4, Math.max(1, fanIntensity + delta));
			renderIntensityBars(fanIntensity);
			const sub = document.getElementById('fanSub');
			if (document.getElementById('fanCard').classList.contains('fan-on')) {
				sub.textContent = 'Running Speed ' + fanIntensity + ' / 4';
			}
			fetch('/fan?intensity=' + fanIntensity)
				.then(res => res.json())
				.then(data => {
					if (typeof data.level === 'number') {
						fanIntensity = data.level;
						renderIntensityBars(fanIntensity);
					}
				})
				.catch(() => {});
		}

		function updateFan(isOn) {
			const card = document.getElementById('fanCard');
			const pill = document.getElementById('fanPill');
			const sub = document.getElementById('fanSub');
			if (isOn) {
				card.classList.add('fan-on');
				pill.className = 'fan-status-pill on';
				pill.textContent = 'ON';
				sub.textContent = 'Running — Speed ' + fanIntensity + ' / 4';
			} else {
				card.classList.remove('fan-on');
				pill.className = 'fan-status-pill off';
				pill.textContent = 'OFF';
				sub.textContent = '';
			}
			renderIntensityBars(fanIntensity);
		}

		renderIntensityBars(fanIntensity);

		function updateLive(data) {
			const t = parseFloat(data.temperature).toFixed(1);
			const h = parseFloat(data.humidity).toFixed(1);

			if (typeof data.level === 'number') {
				fanIntensity = data.level;
			}

			const tEl = document.getElementById('tempVal');
			tEl.className = 'metric-value temp-val ' + tempClass(parseFloat(t));
			tEl.innerHTML = t + '<span class="metric-unit">C</span>';

			const tState = tempBadgeState(parseFloat(t));
			const tBadge = document.getElementById('tempBadge');
			tBadge.textContent = tState[0];
			tBadge.className = 'badge ' + tState[1];

			document.getElementById('humVal').innerHTML = h + '<span class="metric-unit">%</span>';
			const hState = humBadgeState(parseFloat(h));
			const hBadge = document.getElementById('humBadge');
			hBadge.textContent = hState[0];
			hBadge.className = 'badge ' + hState[1];

			readings.push(parseFloat(t));
			if (readings.length > MAX_LOG) readings.shift();

			const mn = Math.min(...readings).toFixed(1);
			const mx = Math.max(...readings).toFixed(1);
			const avg = (readings.reduce((a, b) => a + b, 0) / readings.length).toFixed(1);

			document.getElementById('minTemp').textContent = mn;
			document.getElementById('avgTemp').textContent = avg;
			document.getElementById('maxTemp').textContent = mx;

			document.getElementById('lastUpdated').textContent = 'Last updated ' + new Date().toLocaleTimeString();

			updateFan(!!data.fan);
			pushChart(t, h);
		}

		let simTemp = 27.0;
		let simHum = 58.0;
		let simMode = false;

		function simStep() {
			simTemp = Math.min(45, Math.max(15, simTemp + (Math.random() - 0.48) * 0.6));
			simHum = Math.min(90, Math.max(20, simHum + (Math.random() - 0.48) * 1.2));
			return { temperature: simTemp.toFixed(1), humidity: simHum.toFixed(1), fan: simTemp >= 30, level: fanIntensity };
		}

		function setSimBadge(active) {
			const el = document.getElementById('simBadge');
			if (el) el.style.display = active ? 'inline-flex' : 'none';
		}

		(function seed() {
			for (let i = 0; i < 10; i++) updateLive(simStep());
		})();
		simMode = true;
		setSimBadge(true);

		setInterval(() => {
			fetch('/status')
				.then(res => res.json())
				.then(data => {
					if (simMode) {
						simMode = false;
						setSimBadge(false);
					}
					updateLive(data);
				})
				.catch(() => {
					simMode = true;
					setSimBadge(true);
					updateLive(simStep());
				});
		}, 2000);
	</script>
</body>
</html>
)=====";

void updateLevelLeds() {
	if (!indicatorState) {
		digitalWrite(LED1_PIN, LOW);
		digitalWrite(LED2_PIN, LOW);
		digitalWrite(LED3_PIN, LOW);
		digitalWrite(LED4_PIN, LOW);
		return;
	}

	digitalWrite(LED1_PIN, HIGH);
	digitalWrite(LED2_PIN, level >= 2 ? HIGH : LOW);
	digitalWrite(LED3_PIN, level >= 3 ? HIGH : LOW);
	digitalWrite(LED4_PIN, level >= 4 ? HIGH : LOW);
}

void applyFanState(float temperatureC) {
	indicatorState = temperatureC >= FAN_ON_TEMP_THRESHOLD;
	digitalWrite(INDICATOR_PIN, indicatorState ? HIGH : LOW);
	updateLevelLeds();
}

bool readSensorData(bool forceRead = false) {
	unsigned long currentTime = millis();

	if (!forceRead && hasValidSensorData && (currentTime - lastTempReadTime) < TEMP_READ_INTERVAL) {
		return true;
	}

	float temperatureC = dht.readTemperature();
	float humidity = dht.readHumidity();

	if (isnan(temperatureC) || isnan(humidity)) {
		delay(50);
		temperatureC = dht.readTemperature();
		humidity = dht.readHumidity();
	}

	if (isnan(temperatureC) || isnan(humidity)) {
		Serial.println("Failed to read from DHT11 sensor. Check wiring and the pull-up resistor on DATA.");
		return false;
	}

	lastTempReadTime = currentTime;
	lastTemperatureC = temperatureC;
	lastHumidity = humidity;
	hasValidSensorData = true;

	applyFanState(temperatureC);

	Serial.print("Temperature: ");
	Serial.print(temperatureC, 1);
	Serial.print(" °C | Humidity: ");
	Serial.print(humidity, 1);
	Serial.print(" % | Fan: ");
	Serial.print(indicatorState ? "ON" : "OFF");
	Serial.print(" | Level: ");
	Serial.println(level);

	return true;
}

bool wifiCredentialsConfigured() {
	return strlen(WIFI_SSID) > 0 && strlen(WIFI_PASSWORD) > 0 && String(WIFI_SSID) != "YOUR_WIFI_NAME";
}

void startAccessPoint() {
	WiFi.mode(WIFI_AP);
	WiFi.softAP(AP_SSID, AP_PASSWORD);
	Serial.println("Started fallback Access Point mode.");
	Serial.print("AP SSID: ");
	Serial.println(AP_SSID);
	Serial.print("AP IP address: ");
	Serial.println(WiFi.softAPIP());
}

void connectToWiFi() {
	WiFi.setSleep(false);

	if (!wifiCredentialsConfigured()) {
		Serial.println("Wi-Fi credentials not set. Starting Access Point instead.");
		startAccessPoint();
		return;
	}

	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	Serial.print("Connecting to Wi-Fi");

	unsigned long startAttempt = millis();
	while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < 15000) {
		delay(500);
		Serial.print('.');
	}

	Serial.println();

	if (WiFi.status() == WL_CONNECTED) {
		Serial.print("Connected to ");
		Serial.println(WIFI_SSID);
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
		return;
	}

	Serial.println("Wi-Fi connection failed. Starting Access Point instead.");
	WiFi.disconnect(true, true);
	startAccessPoint();
}

void handleRoot() {
	server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
	if (!readSensorData()) {
		server.send(503, "application/json", "{\"error\":\"sensor_unavailable\"}");
		return;
	}

	char response[128];
	snprintf(
		response,
		sizeof(response),
		"{\"temperature\":%.1f,\"humidity\":%.1f,\"fan\":%s,\"level\":%d}",
		lastTemperatureC,
		lastHumidity,
		indicatorState ? "true" : "false",
		level
	);

	server.send(200, "application/json", response);
}

void handleFanControl() {
	if (server.hasArg("intensity")) {
		int requestedLevel = server.arg("intensity").toInt();
		if (requestedLevel < MIN_LEVEL) {
			requestedLevel = MIN_LEVEL;
		}
		if (requestedLevel > MAX_LEVEL) {
			requestedLevel = MAX_LEVEL;
		}

		level = requestedLevel;
		updateLevelLeds();
		Serial.print("Fan intensity updated from web: ");
		Serial.println(level);
	}

	char response[96];
	snprintf(response, sizeof(response), "{\"fan\":%s,\"level\":%d}", indicatorState ? "true" : "false", level);
	server.send(200, "application/json", response);
}

void handleNotFound() {
	server.send(404, "text/plain", "Not found");
}

void printWebServerAddress() {
	IPAddress ip = WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP();
	Serial.print("Web server IP address: ");
	Serial.println(ip);
	Serial.print("Open: http://");
	Serial.println(ip);
}

void setupServer() {
	server.on("/", handleRoot);
	server.on("/status", HTTP_GET, handleStatus);
	server.on("/fan", HTTP_GET, handleFanControl);
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("Web server started.");
	printWebServerAddress();
}

void handleButtons() {
	unsigned long currentTime = millis();

	bool currentDecState = digitalRead(BUTTON_DEC_PIN);
	bool currentIncState = digitalRead(BUTTON_INC_PIN);

	if (currentDecState != lastDecReading) {
		lastButtonDecChangeTime = currentTime;
		lastDecReading = currentDecState;
	}

	if (currentIncState != lastIncReading) {
		lastButtonIncChangeTime = currentTime;
		lastIncReading = currentIncState;
	}

	if ((currentTime - lastButtonDecChangeTime) >= DEBOUNCE_DELAY && stableDecState != lastDecReading) {
		stableDecState = lastDecReading;
		decPressHandled = false;
	}

	if (!decPressHandled && stableDecState != decIdleState && (currentTime - lastAcceptedButtonTime) >= BUTTON_LOCKOUT_DELAY) {
		lastAcceptedButtonTime = currentTime;
		decPressHandled = true;
		Serial.println("B1 pressed");
		if (level > MIN_LEVEL) {
			level--;
			updateLevelLeds();
			Serial.print("Level decreased to: ");
			Serial.println(level);
		} else {
			Serial.println("Already at minimum level.");
		}
	}

	if ((currentTime - lastButtonIncChangeTime) >= DEBOUNCE_DELAY && stableIncState != lastIncReading) {
		stableIncState = lastIncReading;
		incPressHandled = false;
	}

	if (!incPressHandled && stableIncState != incIdleState && (currentTime - lastAcceptedButtonTime) >= BUTTON_LOCKOUT_DELAY) {
		lastAcceptedButtonTime = currentTime;
		incPressHandled = true;
		Serial.println("B2 pressed");
		if (level < MAX_LEVEL) {
			level++;
			updateLevelLeds();
			Serial.print("Level increased to: ");
			Serial.println(level);
		} else {
			Serial.println("Already at maximum level.");
		}
	}
}

void setup() {
	Serial.begin(115200);

	pinMode(BUTTON_DEC_PIN, INPUT);
	pinMode(BUTTON_INC_PIN, INPUT);

	pinMode(INDICATOR_PIN, OUTPUT);
	pinMode(LED1_PIN, OUTPUT);
	pinMode(LED2_PIN, OUTPUT);
	pinMode(LED3_PIN, OUTPUT);
	pinMode(LED4_PIN, OUTPUT);

	digitalWrite(INDICATOR_PIN, LOW);
	digitalWrite(LED1_PIN, LOW);
	digitalWrite(LED2_PIN, LOW);
	digitalWrite(LED3_PIN, LOW);
	digitalWrite(LED4_PIN, LOW);

	dht.begin();
	delay(2000);

	lastDecReading = digitalRead(BUTTON_DEC_PIN);
	lastIncReading = digitalRead(BUTTON_INC_PIN);
	stableDecState = lastDecReading;
	stableIncState = lastIncReading;
	decIdleState = stableDecState;
	incIdleState = stableIncState;

	Serial.println("ESP32 temperature level controller started.");
	Serial.println("Note: GPIO34 and GPIO35 need external pull-up or pull-down resistors.");
	Serial.println("Buttons auto-detect idle level at startup. Do not hold buttons during boot.");
	Serial.println("DHT11 DATA also needs a 10k pull-up resistor to 3.3V on many modules/sensors.");
	Serial.println("Fan turns on automatically at 30.0 C or above.");

	connectToWiFi();
	readSensorData(true);
	setupServer();
}

void loop() {
	readSensorData();
	handleButtons();
	server.handleClient();
}
