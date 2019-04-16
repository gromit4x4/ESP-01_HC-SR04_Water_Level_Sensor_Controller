/* AN ESP8266 / HC-SR04 WATER LEVEL SENSOR / PUMP CONROLLER
  SENDS HTTP GET REQUESTS TO A SONOFF OR OTHER ESP8266
  DEVICE RUNNING TASMOTA FIRMWARE TO SWITCH
  A TRANSFER PUMP ON WHEN THE WATER TANK IS EMPTY
  AND OFF WHEN THE TANK IS FULL.
  THE APPROXIMATE WATER LEVEL CAN BE CHECKED: http://<device_ip>
  THE SYSTEM CAN BE POLLED FOR ITS CURRENT STATE: http://<device_ip>/state
  THE CONTROL CAN BE ENABLED: http://<device_ip>/enable
  THE CONTROL CAN BE DISABLED: http://<device_ip>/disable
*/

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

// Replace with your network credentials
const char* ssid     = "some_ssid";
const char* password = "some_password";

// Set Tasmota Power ON/OFF Commands - Edit with your device_ip.
const char* tason = "http://192.168.11.118/cm?cmnd=Power%20on";
const char* tasoff = "http://192.168.11.118/cm?cmnd=Power%20off";
const char* tasstate = "http://192.168.11.118/cm?cmnd=Power";

// Set Water Tank Levels in cm
int FULL = 5;
int THREEQUATER = 20;
int HALF = 35;
int ONEQUATER = 50;
int EMPTY = 65;

long duration, distance; // Duration used to calculate distance

// Set web server port number to 80
//WiFiServer server(80);
ESP8266WebServer server(80);

// Auxiliary variables to store the current setting
String enableState = "ENABLED";
String levelState = "EMPTY";
String requestState = "PUMPOFF";
String pumpState = "POWER";
String controlState = "RUNNING";

#define echoPin 2 // Echo Pin
#define trigPin 0 // Trigger Pin

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.hostname("LEVEL_SENSOR");
    delay(500);
    Serial.print(".");
  }

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  server.on("/", root);
  server.on("/enable", enable);
  server.on("/disable", disable);
  server.on("/state", state);
  server.begin();
}

void loop() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.println("***BEGIN**********************************************");
  Serial.println("");
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  Serial.println("");
  HTTPClient http;


  //WiFiClient client = server.available();   // Listen for incoming clients
  server.handleClient();
  if (distance <= FULL && enableState == "ENABLED" && controlState == "RUNNING") {
    Serial.println("TANK FULL, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK FULL";
    http.begin(tasoff); //HTTP Request tasoff
    requestState = "PUMPOFF";
    Serial.println("[HTTP] GET REQUEST-PUMP SHUT OFF... tasoff\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    http.end();
  }
  else if (distance <= FULL && enableState == "ENABLED" && controlState == "NOT RUNNING") {
    Serial.println("TANK FULL, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK FULL";


  }
  else if (distance <= FULL && enableState == "DISABLED") {
    Serial.println("TANK FULL, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK FULL";

  }
  else if (distance <= THREEQUATER) {
    Serial.println("TANK 75% FULL, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK 75% FULL";
  }
  else if (distance <= HALF) {
    Serial.println("TANK 50% FULL, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK 50% FULL";
  }
  else if (distance <= ONEQUATER) {
    Serial.println("TANK 25% FULL, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK 25% FULL";
  }
  else if (distance >= EMPTY && enableState == "ENABLED" && controlState == "NOT RUNNING") {
    Serial.println("TANK EMPTY, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK EMPTY";
    http.begin(tason); //HTTP Request tason
    requestState = "PUMPON";
    Serial.println("[HTTP] GET REQUEST-PUMP ON... tason\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    http.end();
  }
  else if (distance >= EMPTY && enableState == "ENABLED" && controlState == "RUNNING") {
    Serial.println("TANK EMPTY, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK EMPTY";

  }
  else if (distance >= EMPTY && enableState == "DISABLED") {
    Serial.println("TANK EMPTY, SYSTEM " + enableState);
    Serial.println("PUMP " + controlState);
    levelState = "TANK EMPTY";
  }
  http.begin(tasstate); //HTTP Request tasstate
  Serial.println("[HTTP] GET REQUEST-PUMP OPERATING STATE... tasstate\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  pumpState = http.getString();
  http.end();
  Serial.println("tasstate = " + pumpState);
  // Convert Tasmota {"POWER":"ON"} to RUNNING / {"POWER":"OFF"} to NOT RUNNING
  if (pumpState.substring(10, 12) == "ON") {
    controlState = "RUNNING";
    Serial.println("PUMP " + controlState);
  }
  else if (pumpState.substring(10, 13) == "OFF") {
    controlState = "NOT RUNNING";
    Serial.println("PUMP " + controlState);
  }

  // Check Pump Shut Off
  if (enableState == "ENABLED" && requestState == "PUMPOFF" && controlState == "RUNNING") {
    Serial.println("PUMP SHUT OFF REQUEST SENT - PUMP STILL RUNNING!!!!!");
    http.begin(tasoff); //HTTP Request tasoff
    requestState = "PUMPOFF";
    Serial.println("[HTTP] GET REQUEST-PUMP SHUT OFF... tasoff\n");
    Serial.println("");
    // start connection and send HTTP header
    int httpCode = http.GET();
    http.end();
  }
  else if (enableState == "ENABLED" && requestState == "PUMPOFF" && controlState == "NOT RUNNING") {
    Serial.println("PUMP SHUT OFF SUCCESSFULL");
    Serial.println("");
  }

  Serial.println("---END------------------------------------------------");
  Serial.println("");
  Serial.println("");
  delay (5000);
}

void root()
{
  Serial.println("http Level Request");
  server.send(200, "text/plain", levelState);

}

void enable()
{
  enableState = "ENABLED";
  Serial.println("http Enable Request");
  server.send(200, "text/plain", "SYSTEM " + enableState);

}

void disable()
{
  enableState = "DISABLED";
  Serial.println("http Disable Request");
  server.send(200, "text/plain", "SYSTEM " + enableState);
}

void state()
{
  Serial.println("http State Request");
  server.send(200, "text/plain", enableState);
}
