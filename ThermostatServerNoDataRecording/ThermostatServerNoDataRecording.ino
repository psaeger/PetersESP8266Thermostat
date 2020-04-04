//Wifi & Server
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>   // Include the SPIFFS library

// Static IP
IPAddress staticIP(192, 168, 0, 52);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 0, 1);

#ifndef STASSID
#define STASSID "Shop"
#define STAPSK  "Arthur0801"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)

// PushingBox Connection
/*
char serverName[] = "api.pushingbox.com";
boolean lastConnected = false;
char DEVID1[] = "*******";
boolean DEBUG = true;
WiFiClient client;
*/

//Temp Sensor
#include <DHT.h>

#define DHTPIN D1
#define RELAYPIN D2
#define DHTTYPE DHT22

DHT DHT1(DHTPIN, DHTTYPE);

float MostRecentTempRead = 0.0;
const int tempSmoothing = 30;                       // How many readings to take a running average from
float tempReadings[tempSmoothing];                   // the readings from the analog input
int tempReadIndex = 0;                             // the index of the current reading
float tempReadingsTotal = 0.0;
float currentAverageTemp = 0.0;
bool RelayON = false;
//int heatStatus;                                 //Used for pushingbox

//Temperature Range
float LowTemp = 55.0;
float HighTemp = 70.0;

//Temp Check Loop Delay
long previousMillis = 0;
long interval = 1000;

//Send Temp Loop Delay                //How often to send to pushingbox
/*
long previousMillis2 = 0;
long interval2 = 120000;
 */

void setup() {
  Serial.begin(9600);
  Serial.println('\n');
  WiFi.mode(WIFI_STA);
  WiFi.config(staticIP, gateway, subnet, dns);
  WiFi.begin(ssid, password);
  Serial.println("Connecting ...");
  while (WiFi.status() != WL_CONNECTED) {               // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  SPIFFS.begin();                                       // Start the SPI Flash Files System
  server.on("/statusRequest", handleStatusRequest);
  server.on("/tempChange", handleTempChange);
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started\n");
  DHT1.begin();
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, LOW);              // Turn off relay to start

  // Initialize all the temp readings to 60:
  for (int i = 0; i <= tempSmoothing; i++) {
    tempReadings[i] = 60.0;
  }
  tempReadingsTotal = 60.0 * tempSmoothing;
}

void loop(void) {
  server.handleClient();


  /* //Record data to PushingBox
  unsigned long currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 > interval2) {
    previousMillis2 = currentMillis2;
    recordData(DEVID1);
  }
   */

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    updateTempAverage();
    updateRelayStatus(RelayON);
    if (RelayON == true) {
      // Check to see if conditions to shut off are met:
      if (currentAverageTemp > HighTemp) {
        RelayON = false;
        digitalWrite(RELAYPIN, LOW);
        //recordData(DEVID1);             //Pass data to pushingbox
      }
    } else {
      // Check to see if conditions to call for heat are met:
      if (currentAverageTemp < LowTemp) {
        RelayON = true;
        digitalWrite(RELAYPIN, HIGH);
        //recordData(DEVID1);             //Pass data to pushingbox
      }
    }
    Serial.println("LastReading: " + String(MostRecentTempRead));
    Serial.println("Average: " + String(currentAverageTemp));
    Serial.println("RelayStatus: " + String(RelayON));
    Serial.println("");
  }
}

// Loop End

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

// Function to report current temperature and relay status to website
void handleStatusRequest() {
  String
  statusRequest = "{\"temperature\":";
  statusRequest += String(currentAverageTemp);
  statusRequest += ",\"relayStatus\":";
  statusRequest += String(RelayON);
  statusRequest += ",\"lowTemp\":";
  statusRequest += String(LowTemp);
  statusRequest += ",\"highTemp\":";
  statusRequest += String(HighTemp);
  statusRequest += "}";
  server.send(200, "text/plane", String(statusRequest));
  Serial.println(String(statusRequest));
}

// Function to Update Temps
void handleTempChange() {

  if ( !server.hasArg("LowTemp") || !server.hasArg("HighTemp") || server.arg("LowTemp") == NULL || server.arg("HighTemp") == NULL) {//Post Request doesn't have both temps
    server.send(400, "text/html", "400: Invalid Request <br /> Both Temperatures are Required");
    return;
  }

  //report in serial mon results of form submission
  String
  message = "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.println(message);

  //Set new values
  LowTemp = server.arg("LowTemp").toFloat();
  HighTemp = server.arg("HighTemp").toFloat();
  //Report new values
  String
  updateRepsonse = "{\"lowTemp\":";
  updateRepsonse += String(LowTemp);
  updateRepsonse += ",\"highTemp\":";
  updateRepsonse += String(HighTemp);
  updateRepsonse += "}";
  server.send ( 200, "text/plain", String(updateRepsonse) );
}


void updateRelayStatus(bool turnON) {
  if (turnON == true) {
    digitalWrite(RELAYPIN, HIGH);
  } else {
    digitalWrite(RELAYPIN, LOW);
  }
}

// Function to update the average temp with another reading.
void updateTempAverage() {
  // Get a reading from the TEMP module
  MostRecentTempRead = DHT1.readTemperature(true);

  if (isnan(MostRecentTempRead)) {
    Serial.println("Failed to read from DHT sensor!");
    // updateTempAverage(); //Try again when failed to read.
    return;
  }

  // Mix latest reading into the running average
  // remove the last reading
  tempReadingsTotal = tempReadingsTotal - tempReadings[tempReadIndex];
  // put the new reading in
  tempReadings[tempReadIndex] = MostRecentTempRead;
  // add the reading to the total
  tempReadingsTotal = tempReadingsTotal + tempReadings[tempReadIndex];
  // advance to the next position in the array
  tempReadIndex = tempReadIndex + 1;

  // if we're at the end of the array...
  if (tempReadIndex >= tempSmoothing) {
    // ...wrap around to the beginning:
    tempReadIndex = 0;
  }

  // calculate the average
  currentAverageTemp = tempReadingsTotal / tempSmoothing;
}

// Function to send data to pushingbox/googlesheet
/*
void recordData(char devid[]) {
  client.stop(); if (DEBUG) {
    Serial.println("connecting...");
  }
  if (client.connect(serverName, 80)) {
    if (DEBUG) {
      Serial.println("connected");
    }
    if (DEBUG) {
      Serial.println("sending request");
    }
    if (RelayON) {
      heatStatus = 40;
    }
    else {
      heatStatus = 0;
    }
    client.print("GET /pushingbox?devid=");
    client.print(devid);
    client.print("&currentAverageTemp=");
    client.print(currentAverageTemp);
    client.print("&heatStatus=");
    client.print(heatStatus);
    //client.print(RelayON);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverName);
    client.println("User-Agent: Arduino");
    client.println();
  }
  else {
    if (DEBUG) {
      Serial.println("connection failed");
    }
  }
}*/
