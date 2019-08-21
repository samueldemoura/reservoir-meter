#include "ConfigHandler.h"
#include "RequestHandler.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <Ultrasonic.h>
#include <algorithm>
#include <string>

/**
 * These values may be altered depending on your needs.
 */

// Each reservoir that connects to a server must have an unique identifier.
const char *id = "1234";

// Settings for the configuration AP that gets exposed in case the ESP8266 can't connect to WiFi.
const char *ap_name = "reservoirmeter";
const char *ap_password = "2907158928";

// Initialize ultrasonic sensor. Change these if it doesn't match your setup.
Ultrasonic sensor(D2, D4); // trig, echo

/**
 * EVERYTHING BELOW CAN BE LEFT AS IS
 */

// Represents wether device will go into WiFi config mode or regular mode.
bool config_mode = false;

// Initialize the ConfigHandler.
ConfigHandler ch;

// Initialize HTTP client.
HTTPClient http;

// Pointer to web server.
WiFiServer *server;

/**
 * Starts up webserver. Runs once when device fails to connect to WiFi on startup.
 */
void serverSetup() {
  Serial.println(" * Starting WiFi configuration mode...");

  bool ap_result = WiFi.softAP(ap_name, ap_password, false, 4);
  if (ap_result) {
    // WiFi AP was created succesfully, initialize web server.
    Serial.print(" * WiFi AP `");
    Serial.print(ap_name);
    Serial.println("` created. Starting up web server...");

    server = new WiFiServer(80);
    server->begin();

    Serial.print(" * Done. Configuration page is available at http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("/");
  } else {
    // TODO: Treat WiFi creation failure
    Serial.println(" ! Failed to create WiFi AP.");
  }
}

/**
 * Runs while device is in configuration mode (exposing the WiFi config page).
 */
void serverLoop() {
  // Listen for incoming clients.
  WiFiClient client = server->available();

  if (client) {
    RequestHandler rh(client);
    rh.HandleClient();
  }
}

/**
 * Runs upon device startup.
 */
void setup() {
  Serial.begin(115200);

  // Initialize filesystem support.
  SPIFFS.begin();

  // Read configuration file.
  ch.ReadConfig();

  // Try to connect to WiFi.
  Serial.print(" * Connecting to WiFi, ssid: ");
  Serial.print(ch.GetSSID());
  Serial.print(", password: ");
  Serial.print(ch.GetPassword());

  WiFi.begin(ch.GetSSID(), ch.GetPassword());

  unsigned short int tryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    ++tryCount;

    if (tryCount > 25) {
      // Failed to connect to WiFi after a few seconds of trying.
      // Initialize config mode.
      Serial.println("\n ! Failed to connect to WiFi AP.");
      config_mode = true;
      break;
    }
  }

  if (config_mode) {
    // Set up AP and web server to serve WiFi configuration page.
    serverSetup();
  } else {
    // Connected to WiFi properly, no need for any other setup steps.
    Serial.print("\n * Success! IP address is: ");
    Serial.println(WiFi.localIP());
  }
}

/**
 * Default loop function.
 */
void loop() {
  if (config_mode) {
    // In configuration mode, run HTTP server loop.
    serverLoop();
  } else {
    // In regular mode, run sensor read loop.
    sensorLoop();
  }
}

/**
 * Takes a reading from the ultrasonic sensor and POSTs it to the server.
 */
void sensorLoop() {
  delay(500);
  unsigned long reading = sensor.convert(sensor.timing(), Ultrasonic::CM);
  Serial.print(" -> Sensor reading: ");
  Serial.print(reading);
  Serial.println("cm");

  postData(&http, reading);
}

/**
 * POSTs sensor data to server endpoint.
 */
int postData(HTTPClient *http, long value) {
  http->begin(ch.GetEndpoint().c_str());
  http->addHeader("Content-Type", "application/x-www-form-urlencoded");

  char buf[20]; // TODO: this hardcoded size limit makes me worry
  snprintf(buf, 20, "id=%s&value=%f", id, (float)value / 100.f);

  return http->POST(buf);
}
