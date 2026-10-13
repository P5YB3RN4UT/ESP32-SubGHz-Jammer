#include <WiFi.h>
#include <WebServer.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// --- Pin Definitions ---
#define CC1101_GDO0 25
#define CC1101_CS   5
#define CC1101_SCK  18
#define CC1101_MOSI 23
#define CC1101_MISO 19
#define BUTTON_PIN  33  // Physical toggle button (connects to GND)

// --- RGB LED Settings ---
// Set to true if your RGB LED is Common Cathode (HIGH = ON). 
// Set to false if your RGB LED is Common Anode (LOW = ON).
#define COMMON_CATHODE true 

#define LED_R 4
#define LED_G 16
#define LED_B 17

// Helper macros for clean logic regardless of LED type
#define LED_ON  (COMMON_CATHODE ? HIGH : LOW)
#define LED_OFF (COMMON_CATHODE ? LOW : HIGH)

// --- WiFi & Web Server Settings ---
const char* ap_ssid = "㋡";
const char* ap_password = "p5yb3rn4ut"; 
WebServer server(80);

// --- Jammer Settings ---
float currentFrequency = 433.92;
bool isJamming = false;

// --- Stealth Mode Setting ---
bool stealthMode = false; // When true, disables all RGB LED output

// --- Button Debounce Variables ---
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// --- Function Prototypes ---
void startJamming();
void stopJamming();
void handleRoot();
void handleStatus();
void handleToggle();
void handleFrequency();
void handleStealth();
void updateLED();

// --- Compact Retro Terminal HTML UI ---
const char* htmlPage = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>SubGHz Terminal</title>
<style>
  body{background:#000;color:#0f0;font-family:'Courier New',monospace;padding:20px;text-align:center;margin:0}
  .box{border:2px solid #0f0;padding:20px;max-width:400px;margin:0 auto;box-shadow:0 0 15px rgba(0,255,0,0.2)}
  .on{color:#f00;text-shadow:0 0 8px #f00;animation:b 1s infinite}
  .off{color:#0f0}
  @keyframes b{50%{opacity:0.5}}
  select,button{background:#000;color:#0f0;border:1px solid #0f0;padding:12px;font-family:monospace;font-size:1.1em;width:100%;margin:10px 0;cursor:pointer;text-transform:uppercase}
  button:hover{background:#0f0;color:#000}
  .log{text-align:left;font-size:0.85em;margin-top:20px;border-top:1px dashed #0f0;padding-top:10px;height:100px;overflow-y:auto;color:#0a0}
</style></head><body>
<div class="box">
  <h1>SubGHz Jammer v1.0</h1>
  <h2 id="st" class="off">STANDBY</h2>
  <label>TARGET FREQUENCY:</label>
  <select id="fq" onchange="cf()">
    <option value="315.00">315 MHz</option>
    <option value="433.92" selected>433 MHz</option>
    <option value="868.00">868 MHz</option>
    <option value="915.00">915 MHz</option>
  </select>
  <button id="btn" onclick="tj()">[ ACTIVATE ]</button>
  <button id="stlBtn" onclick="ts()">[ STEALTH: OFF ]</button>
  <div class="log" id="log">> System initialized...<br>> Waiting for input...<br></div>
</div>
<script>
  function us(){fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('st').innerText=d.j?'ACTIVE':'STANDBY';
    document.getElementById('st').className=d.j?'on':'off';
    document.getElementById('btn').innerText=d.j?'[ DEACTIVATE ]':'[ ACTIVATE ]';
    document.getElementById('fq').value = d.f;
    document.getElementById('stlBtn').innerText = d.s ? '[ STEALTH: ON ]' : '[ STEALTH: OFF ]';
  })}
  function tj(){fetch('/toggle',{method:'POST'}).then(()=>{us();let j=document.getElementById('st').innerText==='ACTIVE';document.getElementById('log').innerHTML+='> '+(j?'Jamming initiated.':'Jamming terminated.')+'<br>';document.getElementById('log').scrollTop=9999;})}
  function cf(){let f=document.getElementById('fq').value;fetch('/frequency?freq='+f).then(()=>{us();document.getElementById('log').innerHTML+='> Frequency set to '+f+' MHz<br>';document.getElementById('log').scrollTop=9999;})}
  function ts(){fetch('/stealth',{method:'POST'}).then(()=>{us();document.getElementById('log').innerHTML+='> Stealth mode toggled.<br>';document.getElementById('log').scrollTop=9999;})}
  setInterval(us,1000);us();
</script></body></html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize RGB LED pins
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  updateLED(); // Set initial color based on default frequency

  ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);
  ELECHOUSE_cc1101.setGDO0(CC1101_GDO0);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setMHZ(currentFrequency);
  ELECHOUSE_cc1101.setRxBW(270.0);
  ELECHOUSE_cc1101.setDeviation(0);
  ELECHOUSE_cc1101.setPA(12);
  ELECHOUSE_cc1101.SetRx();

  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP Started: " + String(ap_ssid));
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.on("/frequency", HTTP_GET, handleFrequency);
  server.on("/stealth", HTTP_POST, handleStealth); // NEW: Stealth mode toggle route
  server.begin();
  Serial.println("HTTP Server Started");
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      if (currentButtonState == LOW) { // Button pressed
        isJamming = !isJamming;
        if (isJamming) {
          startJamming();
          Serial.println("[PHYSICAL] Jammer: ON");
        } else {
          stopJamming();
          Serial.println("[PHYSICAL] Jammer: OFF");
        }
        updateLED(); // Update LED immediately on physical toggle
      }
    }
  }
  lastButtonState = reading;
  
  // Continuously update LED to handle the non-blocking blinking effect when jamming
  updateLED();
  
  server.handleClient();
}

// --- Web Server Handlers ---
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleStatus() {
  String fStr = String(currentFrequency, 2);
  // Added "s" (stealth) to the JSON response
  String json = "{\"j\":" + String(isJamming ? "true" : "false") + 
                ",\"f\":\"" + fStr + "\"," + 
                "\"s\":" + String(stealthMode ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleToggle() {
  isJamming = !isJamming;
  if (isJamming) {
    startJamming();
    Serial.println("[WEB] Jammer: ON");
  } else {
    stopJamming();
    Serial.println("[WEB] Jammer: OFF");
  }
  updateLED(); // Update LED immediately on web toggle
  server.send(200, "text/plain", "OK");
}

void handleFrequency() {
  if (server.hasArg("freq")) {
    float newFreq = server.arg("freq").toFloat();
    // Using range checking for float comparison safety
    if ((newFreq >= 314.9 && newFreq <= 315.1) ||
        (newFreq >= 433.8 && newFreq <= 434.0) ||
        (newFreq >= 867.9 && newFreq <= 868.1) ||
        (newFreq >= 914.9 && newFreq <= 915.1)) {
      currentFrequency = newFreq;
      Serial.print("[WEB] Frequency changed to: ");
      Serial.println(currentFrequency);
      if (isJamming) {
        stopJamming();
        delay(10);
        startJamming();
      }
      updateLED(); // Update LED immediately to reflect new frequency color
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid Frequency");
}

void handleStealth() {
  stealthMode = !stealthMode;
  updateLED(); // Apply LED state change immediately
  Serial.print("[WEB] Stealth Mode: ");
  Serial.println(stealthMode ? "ON" : "OFF");
  server.send(200, "text/plain", "OK");
}

// --- CC1101 Control Functions ---
void startJamming() {
  Serial.println("Starting jammer...");
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(0); 
  ELECHOUSE_cc1101.setMHZ(currentFrequency);
  ELECHOUSE_cc1101.setPA(12);         
  ELECHOUSE_cc1101.setDeviation(0);
  ELECHOUSE_cc1101.setRxBW(270.0);
  ELECHOUSE_cc1101.SetTx();
  ELECHOUSE_cc1101.SpiWriteReg(0x3E, 0xFF); 
  ELECHOUSE_cc1101.SpiWriteReg(0x35, 0x60); 
}

void stopJamming() {
  Serial.println("Stopping jammer...");
  ELECHOUSE_cc1101.SpiWriteReg(0x35, 0x00);
  ELECHOUSE_cc1101.SetRx(); 
}

// --- RGB LED Control Function ---
void updateLED() {
  // NEW: If stealth mode is active, force all LEDs off and exit early
  if (stealthMode) {
    digitalWrite(LED_R, LED_OFF);
    digitalWrite(LED_G, LED_OFF);
    digitalWrite(LED_B, LED_OFF);
    return;
  }

  int r = 0, g = 0, b = 0;
  
  // Determine color based on selected frequency
  if (currentFrequency >= 314.9 && currentFrequency <= 315.1) {
    r = 1;       // Red
  } else if (currentFrequency >= 433.8 && currentFrequency <= 434.0) {
    g = 1;       // Green
  } else if (currentFrequency >= 867.9 && currentFrequency <= 868.1) {
    b = 1;       // Blue
  } else if (currentFrequency >= 914.9 && currentFrequency <= 915.1) {
    r = 1; b = 1; // Purple (Red + Blue)
  }

  if (isJamming) {
    // Blink the color when jamming (500ms interval)
    if ((millis() / 500) % 2 == 0) {
      digitalWrite(LED_R, r ? LED_ON : LED_OFF);
      digitalWrite(LED_G, g ? LED_ON : LED_OFF);
      digitalWrite(LED_B, b ? LED_ON : LED_OFF);
    } else {
      digitalWrite(LED_R, LED_OFF);
      digitalWrite(LED_G, LED_OFF);
      digitalWrite(LED_B, LED_OFF);
    }
  } else {
    // Solid color when in standby
    digitalWrite(LED_R, r ? LED_ON : LED_OFF);
    digitalWrite(LED_G, g ? LED_ON : LED_OFF);
    digitalWrite(LED_B, b ? LED_ON : LED_OFF);
  }
}