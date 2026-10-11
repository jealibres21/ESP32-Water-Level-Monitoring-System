#define RXp2 16
#define TXp2 17
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

const char* ssid = "WaterLevelSystem";
const char* password = "12345678";

int waterPercent = 0;
int distanceCM = 0;
bool pumpState = false;
String waterStatus = "Waiting for data...";

enum PumpMode { AUTO, MANUAL };
PumpMode currentMode = AUTO;


void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>💧 Water Level Management</title>

  <style>
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(180deg, #0a1f44 0%, #021024 60%, #003366 85%, #004c99 100%);
      color: #ffffff;
      text-align: center;
      padding: 20px;
      margin: 0;
      position: relative;
      overflow: hidden;
    }
    h1 { color: #4FC3F7; margin-bottom: 20px; }
    .card {
      background: rgba(255, 255, 255, 0.1);
      border-radius: 15px;
      padding: 20px;
      margin: 15px auto;
      width: 320px;
      box-shadow: 0 4px 10px rgba(0, 0, 0, 0.3);
    }
    .button { background-color: #00BCD4; color: white; border: none; padding: 12px 30px; border-radius: 8px; cursor: pointer; }
    .button:hover { background-color: #0097A7; }
    .button-container { display: flex; justify-content: center; gap: 15px; flex-wrap: wrap; }
  </style>
</head>
<body>

  <h1>💧 Water Level Management</h1>

  <div class="card">
    <div class="label">Water Level</div>
    <div class="value" id="waterLevel">--%</div>
    <div>Distance: <span id="distance">-- cm</span></div>
  </div>

  <div class="card">
    <div class="label">Pump Status</div>
    <div class="status" id="pumpStatus">Pump: --</div>
  </div>

  <div class="card">
    <div class="label">Mode</div>
    <div class="status" id="pumpMode">AUTO</div>
  </div>

  <div class="button-container" id="buttonContainer">
    <button class="button" onclick="setAutoMode()">AUTO Mode</button>
    <button class="button" onclick="setManualMode()">Manual</button>
  </div>

  <script>
  function updateData() {
    fetch('/data')
      .then(res => res.json())
      .then(data => {
        document.getElementById('waterLevel').textContent = data.percent + '%';
        document.getElementById('distance').textContent = data.distance + ' cm';
        document.getElementById('pumpStatus').textContent = (data.pump ? 'ON' : 'OFF');
        document.getElementById('pumpStatus').style.color = data.pump ? '#00E676' : '#FF7043';
        document.getElementById('pumpMode').textContent = data.mode;
        document.getElementById('pumpMode').style.color = data.mode == 'AUTO' ? '#00E676' : '#FF7043';

        const buttonContainer = document.getElementById('buttonContainer');
        if (data.mode === 'AUTO') {
          buttonContainer.innerHTML = `
            <button class="button" style="background-color:#00E676;" onclick="setAutoMode()">AUTO Mode</button>
            <button class="button" style="background-color:#00BCD4;" onclick="setManualMode()">Manual</button>
          `;
        } else {
          const pumpOnColor = data.pump ? '#00E676' : '#00BCD4';
          const pumpOffColor = !data.pump ? '#FF7043' : '#00BCD4';
          buttonContainer.innerHTML = `
            <button class="button" style="background-color:#FFB300;" onclick="setAutoMode()">AUTO Mode</button>
            <button class="button" style="background-color:${pumpOnColor};" onclick="setPumpOn()">Pump ON</button>
            <button class="button" style="background-color:${pumpOffColor};" onclick="setPumpOff()">Pump OFF</button>
          `;
        }
      })
      .catch(e => console.error('Error fetching data:', e));
  }

  function setAutoMode() { fetch('/pump/auto').then(updateData); }
  function setManualMode() { fetch('/pump/manual').then(updateData); }
  function setPumpOn() { fetch('/pump/on').then(updateData); }
  function setPumpOff() { fetch('/pump/off').then(updateData); }

  setInterval(updateData, 500);
  updateData();
  </script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}


void handleData() {
  String json = "{";
  json += "\"percent\":" + String(waterPercent) + ",";
  json += "\"distance\":" + String(distanceCM) + ",";
  json += "\"pump\":" + String(pumpState ? "true" : "false") + ",";
  json += "\"status\"😕"" + waterStatus + "\"";
  json += ",\"mode\"😕"" + String(currentMode == AUTO ? "AUTO" : "MANUAL") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);

  WiFi.softAP(ssid, password);
  Serial.println("✅ WiFi AP started!");
  Serial.println("📶 Connect to: " + String(ssid));
  Serial.println("🌐 Open: http://192.168.4.1");
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/pump/off", handlePumpOff);
  server.on("/pump/auto", handlePumpAuto);
  server.on("/pump/manual", handlePumpManual);
  server.on("/pump/on", handlePumpOn);

  server.begin();
}

void handlePumpOff() {
  pumpState = false;
  currentMode = MANUAL;        
  Serial2.println("OFF");      
  server.send(200, "text/plain", "Pump turned OFF (Manual Mode)");

}

void handlePumpAuto() {
  currentMode = AUTO;
  Serial2.println("AUTO");     
  server.send(200, "text/plain", "Automatic mode enabled");
}

void handlePumpManual() {
  currentMode = MANUAL;
  Serial2.println("MANUAL");  
  server.send(200, "text/plain", "Manual mode enabled");
}

void handlePumpOn() {
  if (currentMode == MANUAL) {
    pumpState = true;
    Serial2.println("ON");
    server.send(200, "text/plain", "Pump turned ON (Manual Mode)");
  } else {
    server.send(200, "text/plain", "Switch to manual mode first");
  }
}


void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
     Serial.println("💾 Raw data received: [" + data + "]");
     data.trim();
    
    if (data.length() > 0) {

      int firstComma = data.indexOf(',');
      int secondComma = data.indexOf(',', firstComma + 1);
      int thirdComma = data.indexOf(',', secondComma + 1);

      if (firstComma > 0 && secondComma > firstComma && thirdComma > secondComma) {
        String percentStr = data.substring(0, firstComma);
        waterPercent = percentStr.toInt(); 

        String distanceStr = data.substring(firstComma + 1, secondComma);
        distanceCM = distanceStr.toInt();

        String pumpStr = data.substring(secondComma + 1, thirdComma);
        pumpStr.trim();
        pumpState = (pumpStr.equalsIgnoreCase("ON"));

        waterStatus = data.substring(thirdComma + 1);
        waterStatus.trim();
        
        // Log the successfully parsed data
        Serial.print("📨 Received: ");
        Serial.print(waterPercent); Serial.print("%, ");
        Serial.print(distanceCM); Serial.print("cm, ");
        Serial.print(pumpState ? "ON" : "OFF"); Serial.print(", ");
        Serial.println(waterStatus);

      } else {
        Serial.println("⚠️ Received malformed data: " + data);
      }
    }
  }
  server.handleClient();
}