#include <string>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ultrasonic.h>

// Represents wether device will go into WiFi config mode or regular usage.
bool config_mode = false;

// Replace these with your WiFi network settings.
String *ssid;
String *password;

// Replace the IP in here with the IP address of your web server.
String *url;

// Each reservoir must have an unique identifier.
const char *id = "1234";

// Initialize ultrasonic sensor
Ultrasonic sensor(D2, D4); // trig, echo

// Initialize HTTP client
HTTPClient http;

// Prepare pointer to webserver.
WiFiServer *server;

/**
   Start up webserver. Runs when device fails to connect to WiFi on startup.
*/
void serverSetup() {
    bool wifiResult = WiFi.softAP("reservoirmeter", "2907158928", false, 2);
    if (wifiResult) {
        server = new WiFiServer(80);
        server->begin();
        Serial.print(" * Config page listening at http://");
        Serial.print(WiFi.softAPIP());
        Serial.println("/");
    } else {
        // TODO: Treat WiFi creation failure
        Serial.println(" ! Failed to create WiFi softspot");
    }
  }

/**
   Loop that runs while device is in configuration mode (exposing WiFi config page)
*/
void serverLoop() {
    // Listen for incoming clients
    WiFiClient client = server->available();

    std::string header = "";
    std::string currentLine = "";
    bool isPOST = false;

    if (client) {
        while (client.connected()) {
            // Test if there are incoming bytes from client
            if (client.available()) {
                char chr = client.read();
                header += chr;

                if (chr == '\n') {
                    // Serial.println(" * Streaming data from client: ");
                    // Serial.println(header.c_str());
                    Serial.println(" * Incoming client line: ");
                    Serial.println(currentLine.c_str());
                  
                    if (currentLine.find("POST") == 0) {
                        // This is a POST request coming in.
                        isPOST = true;
                        Serial.println(" * Detected incoming POST request");
                    }
                    if (isPOST && currentLine.find("name=") != -1) {
                        // currentLine has POST data in it
                        std::string ssid = currentLine.substr(currentLine.find("name="), currentLine.find("&password=") - 1);
                        std::string password = currentLine.substr(currentLine.find("&password="), currentLine.find("&url=") - 1);
                        std::string url = currentLine.substr(currentLine.find("&url="));

                        Serial.print(" * Received WiFi config: ");
                        Serial.print(ssid.c_str());
                        Serial.print(", ");
                        Serial.println(password.c_str());
                        Serial.print(" * Received server IP: ");
                        Serial.println(url.c_str());

                        // Write to file
                        writeConfig(ssid, password, url);
                    }
                    else if (currentLine.length() == 0) {
                        // Two newlines in a row = client HTTP request
                        // ended. Send response:
                        if (isPOST) {
                          // TODO: treat POST end
                          Serial.println(" * Incoming POST data from client: ");
                          Serial.println(header.c_str());

                          client.println(
                            "HTTP/1.1 200 OK\n"
                            "Content-type:text/html\n"
                            "Connection: close\n");
                          break;
                        } else {
                          Serial.println(" * Incoming non-POST data from client: ");
                          Serial.println(header.c_str());
  
                          client.println(
                            "HTTP/1.1 200 OK\n"
                            "Content-type:text/html\n"
                            "Connection: close\n");
  
                          // Send the HTML of the config page.
                          client.println(
                             "<!DOCTYPE html><html><head><title>Title</title><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/uikit/3.1.6/css/uikit.min.css\" /> <script src=\"https://cdnjs.cloudflare.com/ajax/libs/uikit/3.1.6/js/uikit.min.js\"></script> </head><body><nav class=\"uk-navbar-container uk-navbar-transparent uk-background-primary uk-light\" uk-navbar><div class=\"uk-navbar-left\"><ul class=\"uk-navbar-left\"><li class=\"uk-navbar-item uk-logo\">reservoir-meter</li></ul></div> </nav><div class=\"uk-container uk-container-small\"><form class=\"uk-margin-top\" action=\"/\", method=\"POST\"><fieldset class=\"uk-fieldset\"><legend class=\"uk-legend\">WiFi Configuration</legend><div class=\"uk-margin\"> <input class=\"uk-input\" type=\"text\" name=\"name\" placeholder=\"WiFi AP name\"></div><div class=\"uk-margin\"> <input class=\"uk-input\" type=\"password\" name=\"password\" placeholder=\"WiFi password\"></div><div class=\"uk-margin\"> <input class=\"uk-input\" type=\"text\" name=\"url\" placeholder=\"Server IP\"></div><button class=\"uk-button uk-button-primary\" type=\"submit\">Save</button></fieldset></form></div></body></html>"
                          );
  
                          // Done.
                          break;
                        }
                    }
                    else {
                      // Got a regular newline.
                      currentLine = "";
                    }
                } else if (chr != '\r') {
                    // Got something that's not \n or \r, add to currentLine
                    currentLine += chr;
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

  // SPIFFS
  SPIFFS.begin();

  // Read config from file
  readConfig();

  // Connect to WiFi
  Serial.print(" * Connecting to WiFi");
  WiFi.begin(ssid->c_str(), password->c_str());
  unsigned short int tryCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    ++tryCount;

    if (tryCount > 5) {
      // Failed to connect to WiFi after a few seconds of trying.
      // Initialize config mode.
      Serial.println(" ! Failed to connect to WiFi AP. Starting config mode...");

      config_mode = true;
      break;
    }
  }

  if (config_mode) {
      serverSetup();
  } else {
    Serial.print(" * Success! IP address is: ");
    Serial.println(WiFi.localIP());
  }
}

/**
   Default loop function.
*/
void loop() {
    if (!config_mode) {
      sensorLoop();
    } else {
      serverLoop();
    }
}

/**
   Loop that runs after device is connected to a WiFi network.
*/
void sensorLoop() {
    delay(500);
    long reading = sensor.convert(sensor.timing(), Ultrasonic::CM);
    Serial.print(" > Reading: ");
    Serial.println(reading);

    postData(&http, reading);
}

/**
   POSTs sensor data to server endpoint.
*/
int postData(HTTPClient *http, long value)
{
    http->begin(url->c_str());
    http->addHeader("Content-Type", "application/x-www-form-urlencoded");

    char buf[20];
    snprintf(buf, 20, "id=%s&value=%f", id, (float)value / 100.f);

    return http->POST(buf);
}

/**
   Functions to manipulate config file.
*/
void readConfig() {
  Serial.println(" * Attempting to open config file for reading...");
  delay(1000);

  File configFile = SPIFFS.open("/config", "r");

  if (configFile) {
    Serial.println(" * Opened config file. Reading values...");
    ssid = new String(configFile.readStringUntil('\n'));
    password = new String(configFile.readStringUntil('\n'));
    url = new String(configFile.readStringUntil('\n'));
    configFile.close();
  } else {
    // Use defaults while config does not exist
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
