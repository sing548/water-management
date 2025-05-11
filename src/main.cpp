#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "Guenther5";
const char* password = "DariomagBananen1";
String newHostName = "ESP_Water_Server";

ESP8266WebServer server(80);


int humidityPin1 = A0;
int humidityPin2 = D5;
int humidityPin3 = D7;
int pumpPin = D1;

bool pumpActive = false;
unsigned long pumpStartTime = 0;
unsigned int pumpDuration = 0;

const char* user = "Pumpöö";
const char* pumpPassword = "BitteWasserDamitMeinDoofenPflanzenNichtStärben";

void setupServer();
void restServerRouting();
void handleNotFound();
void getShowHumidity();
void postStartPump();

String getSensorValue1();
String getSensorValue2();
String getSensorValue3();

void setup() {
  Serial.begin(115200);
  Serial.println("Setup...");

  setupServer();


  pinMode(humidityPin1, INPUT);
  pinMode(humidityPin2, INPUT);
  pinMode(humidityPin3, INPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);  
}

void setupServer() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(newHostName.c_str());
  WiFi.begin(ssid, password);

  Serial.println("");

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  restServerRouting();
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void restServerRouting() {
  server.on(F("/"), HTTP_GET, getShowHumidity);
  server.on(F("/startPump"), HTTP_POST, postStartPump);
}

void handleNotFound() {
  String message = "File not Found \n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void getShowHumidity() {
  String value1 = getSensorValue1();
  String value2 = getSensorValue2();
  String value3 = getSensorValue3();

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Sensor Dashboard</title>
    <style>
      body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: #f0f4f8;
        color: #333;
        text-align: center;
        padding: 50px;
      }
      .card {
        background: white;
        max-width: 450px;
        margin: auto;
        padding: 25px;
        border-radius: 12px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
      }
      h1 {
        font-size: 2rem;
        margin-bottom: 20px;
      }
      .sensor {
        font-size: 1.2rem;
        color: #444;
        margin: 10px 0;
      }
      button {
        padding: 12px 24px;
        font-size: 1rem;
        background-color: #007BFF;
        color: white;
        border: none;
        border-radius: 8px;
        cursor: pointer;
        margin-top: 30px;
        transition: background-color 0.3s ease;
      }
      button:hover {
        background-color: #0056b3;
      }
    </style>
  </head>
  <body>
    <div class="card">
      <h2>Start Pump</h2>
      <form action="/startPump" method="POST">
        <button name="duration" value="15" type="submit">15 Second Pump</button>
        <button name="duration" value="30" type="submit">30 Second Pump</button>
        <button name="duration" value="60" type="submit">1 Minute Pump</button>
      </form>

      <form action="/startPump" method="POST" style="margin-top:20px;">
        <input type="number" name="duration" placeholder="Custom (sec)" min="1" max="180" required>
        <button type="submit">Start Custom</button>
      </form>

    </div>
  </body>
  </html>

  )rawliteral";

  server.send(200, "text/html", html);
}

void postStartPump() {
  
  if (!server.authenticate(user, pumpPassword)) {
    server.requestAuthentication();
    return;
  }

  int durationSec = server.hasArg("duration") ? server.arg("duration").toInt() : 4;

  // Cap the duration to a safe maximum, e.g., 300 seconds
  if (durationSec < 1) durationSec = 1;
  if (durationSec > 180) durationSec = 180;

  digitalWrite(pumpPin, HIGH);
  pumpActive = true;
  pumpStartTime = millis();
  pumpDuration = durationSec * 1000;
  Serial.println("Pump turned on!");

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Pump Running</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #f0f4f8;
      text-align: center;
      padding: 50px;
    }
    .spinner {
      margin: 40px auto;
      width: 50px;
      height: 50px;
      border: 6px solid #ccc;
      border-top: 6px solid #007BFF;
      border-radius: 50%;
      animation: spin 1s linear infinite;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    p {
      font-size: 1.2rem;
      margin-top: 20px;
      color: #444;
    }
  </style>
  <script>
    let seconds = )rawliteral" + String(durationSec + 1) + R"rawliteral(;
    function updateCountdown() {
      document.getElementById("countdown").innerText = seconds;
      if (seconds === 0) {
        window.location.href = "/";
      } else {
        seconds--;
        setTimeout(updateCountdown, 1000);
      }
    }
    window.onload = updateCountdown;
  </script>
</head>
<body>
  <h1>Pump started for )rawliteral" + String(durationSec) + R"rawliteral( seconds</h1>
  <div class="spinner"></div>
  <p>Redirecting in <span id="countdown">)rawliteral" + String(durationSec + 1) + R"rawliteral(</span> seconds...</p>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

String getSensorValue1() {
  int sensorValue = analogRead(humidityPin1);
  Serial.println("Current humidity sensor 1 (A0): " + String(sensorValue));
  return String(String(sensorValue));
}

String getSensorValue2() {
  int sensorValue = digitalRead(humidityPin2);
  Serial.println("Current humidity sensor 2 (D5): " + String(sensorValue));
  String returnValue;

  if (sensorValue) {
    returnValue = "1";
  } else {
    returnValue = "0";
  }
  return returnValue;
}

String getSensorValue3() {
  int sensorValue = digitalRead(humidityPin3);
  Serial.println("Current humidity sensor 3 (D7): " + String(sensorValue));
  String returnValue;

  if (sensorValue) {
    returnValue = "1";
  } else {
    returnValue = "0";
  }
  return returnValue;
}

void loop() {
  server.handleClient();

  if (pumpActive && millis() - pumpStartTime >= pumpDuration) {
    digitalWrite(pumpPin, LOW);
    pumpActive = false;
    Serial.println("Pump turned off!");
    pumpDuration = 0;
  }
}
