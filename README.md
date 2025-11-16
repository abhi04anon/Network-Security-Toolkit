Legal Disclaimer

This tool is for educational and authorized testing purposes only.

Only use on networks you own or have explicit written permission to test

Unauthorized use may be illegal in your jurisdiction

The developers are not responsible for any misuse

Use responsibly and ethically

----

🚀 Features
Deauthentication Attacks - Disconnect devices from target networks

Beacon Flood - Spam fake network advertisements

Probe Flood - Send mass probe requests

Fake AP - Create replica access points

Captive Portal - Credential harvesting portals

Real-time Web Interface - Modern responsive dashboard

Status Monitoring - Live attack status and system metrics

----

📋 Prerequisites
Hardware
ESP8266 Board (NodeMCU, Wemos D1 Mini, etc.)

Micro USB cable

Computer with Arduino IDE

Software
Arduino IDE 1.8.0+

ESP8266 Board Support

```🔧 Installation
1. Install Arduino IDE
Download and install Arduino IDE from arduino.cc

2. Install ESP8266 Support
Open Arduino IDE

Go to File > Preferences

Add to Additional Board Manager URLs: http://arduino.esp8266.com/stable/package_esp8266com_index.json

Go to Tools > Board > Board Manager

Search for "esp8266" and install

3. Required Libraries
The following libraries are included with ESP8266 board package:

ESP8266WiFi

DNSServer

ESP8266WebServer
```
----

📦 Setup
1. Hardware Setup
Connect ESP8266 to computer via USB

Select correct board in Tools > Board (NodeMCU 1.0)

Select correct port in Tools > Port

2. Upload Code
Open WiFi_Security_Toolkit.ino

Click Upload

Open Serial Monitor (115200 baud)

3. First Run
After upload, you should see:

```🚀 WiFi Security Toolkit Starting...
===================================
✅ System Ready!
📶 AP: Security_Scanner
🔑 Pass: scan12345
🌐 URL: http://192.168.4.1
💾 Free Memory: XXXXX bytes
```
----
# Usage Guide

## Basic Operation

### 1. Initial Setup
1. Power on the ESP8266
2. Connect to WiFi: `Security_Scanner` (password: `scan12345`)
3. Open browser: `http://192.168.4.1`

### 2. Network Scanning
- Click "Rescan Networks" to refresh
- Networks appear with signal strength indicators
- Select target network by clicking "Select"

### 3. Attack Operations

#### Deauthentication Attack
- Disconnects devices from target network
- Select target first
- Click "Start Deauth"
- Monitor status indicators

#### Captive Portal (Evil Twin)
- Creates fake login page mimicking target
- Captures entered credentials
- Requires target network selection

#### Beacon Flood
- Broadcasts fake network advertisements
- No target selection required
- Creates network congestion

#### Fake AP
- Spoofs legitimate access point
- Uses target network's SSID
- Optional password protection

## Advanced Features

### Status Monitoring
- Real-time operation status
- Memory usage statistics
- Uptime tracking
- Network count

### Credential Capture
- Captured passwords appear in web interface
- Includes SSID, BSSID, and timestamp
- Automatic logging

### Emergency Stop
- "Emergency Stop" button halts all operations
- Returns to default AP mode
- Clears all attack states

## Best Practices

### For Testing
1. Always test in controlled environments
2. Use your own networks for testing
3. Document all testing activities
4. Obtain proper authorization

### Performance Tips
- Keep ESP8266 close to target for better range
- Monitor device temperature during extended use
- Use external power for long sessions
- Clear captured data regularly

## Safety Notes

- Device may get warm during extended attacks
- Some attacks may trigger network security systems
- Be aware of battery life during portable operation
