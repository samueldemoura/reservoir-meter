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

// Configuration values. Will be read from file on startup.
String *ssid;
String *password;
String *url;

// Represents wether device will go into WiFi config mode or regular mode.
bool config_mode = false;

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
  // Listen for incoming clients
  WiFiClient client = server->available();

  std::string header = "";
  std::string currentLine = "";

  bool isPOST = false; // wether current request is a POST or GET
  bool readingBody = false; // wether we're already reading the body or still at the headers
  int contentLength; // size of body, in case of a POST request

  if (client) {
    while (client.connected()) {
      // Test if there are incoming bytes from client
      if (client.available()) {
        char chr = client.read();
        header += chr;

        if (chr == '\n') {
          // End of line. Time to process current line.
          if (currentLine.find("POST") == 0) {
            // This is a POST request coming in.
            isPOST = true;
            Serial.println(" * Detected incoming POST request.");
          }

          if (isPOST && !readingBody && currentLine.find("Content-Length: ") == 0) {
            // Received Content-Length. Parse it.
            Serial.print(" * Detected Content-Length: ");
            contentLength = std::atoi(currentLine.substr(16).c_str());
            Serial.println(contentLength);

            // Erase current line buffer.
            currentLine = "";
          } else if (isPOST && currentLine.length() == 0) {
            // Body of POST request starts here.
            Serial.println(" * Started reading body...");
            readingBody = true;
            currentLine = "";
          } else if (!isPOST && currentLine.length() == 0) {
            // Empty line received = GET HTTP request ended. Send response:
            Serial.println(" * Incoming non-POST data from client: ");
            Serial.println(header.c_str());

            // Send response headers.
            client.println(
                "HTTP/1.1 200 OK\n"
                "Content-type:text/html\n"
                "Connection: close\n"
            );

            // Send the HTML of the config page.
            client.println(
                "<!DOCTYPE html><html><head><title>Title</title><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/uikit/3.1.6/css/uikit.min.css\" /> <script src=\"https://cdnjs.cloudflare.com/ajax/libs/uikit/3.1.6/js/uikit.min.js\"></script> </head><body><nav class=\"uk-navbar-container uk-navbar-transparent uk-background-primary uk-light\" uk-navbar><div class=\"uk-navbar-left\"><ul class=\"uk-navbar-left\"><li class=\"uk-navbar-item uk-logo\">reservoir-meter</li></ul></div> </nav><div class=\"uk-container uk-container-small\"><form class=\"uk-margin-top\" action=\"/\", method=\"POST\"><fieldset class=\"uk-fieldset\"><legend class=\"uk-legend\">WiFi Configuration</legend><div class=\"uk-margin\"> <input class=\"uk-input\" type=\"text\" name=\"name\" placeholder=\"WiFi AP name\"></div><div class=\"uk-margin\"> <input class=\"uk-input\" type=\"password\" name=\"password\" placeholder=\"WiFi password\"></div><div class=\"uk-margin\"> <input class=\"uk-input\" type=\"text\" name=\"url\" placeholder=\"Server IP\"></div><button class=\"uk-button uk-button-primary\" type=\"submit\">Save</button></fieldset></form></div></body></html>"
            );

            // Done.
            break;
          } else {
            // Got a regular newline.
            currentLine = "";
          }
        } else if (chr != '\r') {
          // Got something that's not \n or \r, add to currentLine
          currentLine += chr;

          // And, if this is a POST request and we're already reading the body, test if size of
          // the current line matches the Content-Length.
          if (isPOST && readingBody && currentLine.length() == contentLength) {
            // Reached end of POST request body. Parse data.
            int ssidLength = currentLine.find("&password=") - 5;
            int passwordLength = (currentLine.find("&url=")) - (currentLine.find("&password=") + 10);

            std::string ssid = currentLine.substr(currentLine.find("name=") + 5, ssidLength);
            std::string password = currentLine.substr(currentLine.find("&password=") + 10, passwordLength);
            std::string url = currentLine.substr(currentLine.find("&url=") + 5);

            // Append '/api/data' to url string
            if (url.back() != '/') {
              url += '/';
            }
            url += "api/data";

            // Treat spaces in WiFi ssid
            std::replace(ssid.begin(), ssid.end(), '+', ' ');

            Serial.print(" * Received WiFi config: ");
            Serial.print(ssid.c_str());
            Serial.print(", ");
            Serial.println(password.c_str());
            Serial.print(" * Received server IP: ");
            Serial.println(url.c_str());

            // Write to file
            writeConfig(ssid, password, url);

            // Done.
            // TODO: Close connection properly.
            break;
          }
        }
      }
    }

    client.stop();
    Serial.println(" * Client disconnected.");
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
  readConfig();

  // Try to connect to WiFi.
  Serial.print(" * Connecting to WiFi, ssid: ");
  Serial.print(*ssid);
  Serial.print(", password: ");
  Serial.print(*password);
  
  WiFi.begin(*ssid, *password);

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
    Serial.print(" * Success! IP address is: ");
    Serial.println(WiFi.localIP());
  }
}

/**
 * Default loop function.
 */
void loop() {
  if (!config_mode) {
    sensorLoop();
  } else {
    serverLoop();
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
  http->begin(url->c_str());
  http->addHeader("Content-Type", "application/x-www-form-urlencoded");

  char buf[20]; // TODO: this hardcoded size limit makes me worry
  snprintf(buf, 20, "id=%s&value=%f", id, (float)value / 100.f);

  return http->POST(buf);
}

/**
 * Functions to manipulate the configuration file.
 */
void readConfig() {
  Serial.println(" * Attempting to open config file for reading...");
  File configFile = SPIFFS.open("/config", "r");

  if (configFile) {
    Serial.println(" * Opened config file. Reading values...");

    ssid = new String(configFile.readStringUntil('\n'));
    password = new String(configFile.readStringUntil('\n'));
    url = new String(configFile.readStringUntil('\n'));

    ssid->trim();
    password->trim();
    url->trim();

    configFile.close();
  } else {
    Serial.println(" * Failed to open config file. Using default values...");

    ssid = new String("fakessid");
    password = new String("fakepassword");
    url = new String("fakeurl");
  }
}

void writeConfig(std::string ssid, std::string password, std::string url) {
  File configFile = SPIFFS.open("/config", "w");

  configFile.println(ssid.c_str());
  configFile.println(password.c_str());
  configFile.println(url.c_str());

  configFile.close();
}
