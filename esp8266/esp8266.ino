#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ultrasonic.h>

// Represents wether device will go into WiFi config mode or regular usage.
bool config_mode = false;

// Replace these with your WiFi network settings.
const char *ssid = "Redmi note 5";
const char *password = "123456789";

// Replace the IP in here with the IP address of your web server.
const char *url = "http://192.168.43.222:3000/api/data";

// Each reservoir must have an unique identifier.
const char *id = "1234";

// Initialize ultrasonic sensor
Ultrasonic sensor(D2, D4); // trig, echo

// Initialize HTTP client
HTTPClient http;

// Prepare pointer to webserver.
WiFiServer *server = nullptr;

/**
 * Start up webserver. Runs when device fails to connect to WiFi on startup.
 */
void serverSetup() {
    server = new WiFiServer(80); 
}

/**
 * Loop that runs while device is in configuration mode (exposing WiFi config page)
 */
void serverLoop() {
    // Listen for incoming clients
    WiFiClient client = server->available();

    String header = "";
    String currentLine = "";
    
    if (client) {
        while (client.connected()) {
            // Test if there are incoming bytes from client
            if (client.available()) {
                char chr = client.read();
                header += chr;

                if (chr == '\n') {
                    if (currentLine.length() == 0) {
                        // Two newlines in a row = client HHTP request
                        // ended. Send response:
                        Serial.println(" * Incoming data from client: ");
                        Serial.println(header);
                        
                        client.println(
                          "HTTP/1.1 200 OK\n"
                          "Content-type:text/html\n"
                          "Connection: close\n");

                        // TODO: Send HTML page here

                        // Done.
                        break;
                    } else {
                      // Got a regular newline.
                      currentLine = "";
                    }
                } else if (chr != '\r') {
                    // Got something that's not \n or \r, add to currentLine
                    currentLine += chr;
                }
            }
        }

        Serial.println(" * Client disconnected.");
        client.stop();
    }
}

/**
 * Runs upon device startup.
 */
void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    Serial.print(" * Connecting to WiFi");
    WiFi.begin(ssid, password);
    unsigned short int tryCount = 0;

    while (WiFi.status() != WL_CONNECTED) {
        delay(200);
        Serial.print(".");
        ++tryCount;

        if (tryCount > 5) {
            // Failed to connect to WiFi after 10 seconds of trying.
            // Initialize config mode.
            Serial.println(" ! Failed to connect to WiFi AP. Starting config mode...");
            
            config_mode = true;
            serverSetup();
        }
    }

    Serial.println("");
    Serial.print(" * Success! IP address is: ");
    Serial.println(WiFi.localIP());
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
 * Loop that runs after device is connected to a WiFi network.
 */
void sensorLoop() {
    delay(500);
    long reading = sensor.convert(sensor.timing(), Ultrasonic::CM);
    Serial.print(" > Reading: ");
    Serial.println(reading);

    postData(&http, reading);
}

/**
 * POSTs sensor data to server endpoint.
 */
int postData(HTTPClient *http, long value)
{
    http->begin(url);
    http->addHeader("Content-Type", "application/x-www-form-urlencoded");

    char buf[20];
    snprintf(buf, 20, "id=%s&value=%f", id, (float)value / 100.f);

    return http->POST(buf);
}
