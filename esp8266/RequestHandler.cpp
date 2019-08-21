#define _GLIBCXX_USE_C99 1
#define REQUEST_ERROR -1
#define REQUEST_ONGOING 0
#define REQUEST_FINISHED_GET 1
#define REQUEST_FINISHED_POST 2

#include "RequestHandler.h"
#include "ConfigHandler.h"
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <string>

/**
 * Constructor.
 */
RequestHandler::RequestHandler(WiFiClient client) {
  this->client = client;

  this->body = "";
  this->chr = 0;
  this->content_length = 0;
  this->cur_line = "";

  this->request_is_get = false;
  this->request_is_post = false;
  this->reading_request_body = false;
}

/**
 * Handles incoming data for a single client.
 */
unsigned char RequestHandler::HandleClient() {
  while (this->client.connected()) {
    if (this->client.available()) {
      // We have incoming bytes!
      this->chr = this->client.read();

      unsigned char result;
      if (this->chr == '\n') {
        // Reached EOL: time to process the current line.
        result = this->HandleIncomingLine();

        // Clear line buffer
        this->cur_line = "";
      } else if (this->chr != '\r') {
        // Neither \n nor \r, add to current line buffer...
        this->cur_line += this->chr;

        // ...and treat it.
        result = this->HandleIncomingChar();
      }

      // Treat possible errors or request end.
      switch (result) {
      case REQUEST_ERROR:
        // Send error response to client.
        client.println(
            "HTTP/1.1 400 ERROR\n"
            "Connection: close\n");
        goto disconnect;
      case REQUEST_FINISHED_GET:
        // Already sent response, just need to close connection.
        goto disconnect;
      case REQUEST_FINISHED_POST:
        // Send OK response page.
        client.println(
            "HTTP/1.1 200 OK\n"
            "Content-type:text/html\n"
            "Connection: close\n");
        client.println(
            "Your settings were saved. Please reboot the ESP8266.");
        goto disconnect;
        case REQUEST_ONGOING:
        default:
        // Do nothing.
        break;
      }
    }
  }

// Client disconnected.
disconnect:
  client.stop();
  Serial.println(" * Client disconnected.");
}

/**
 * Handles a single char from a client's request.
 */
unsigned char RequestHandler::HandleIncomingChar() {
  if (this->request_is_post && this->reading_request_body) {
    // Reading body of POST request...
    this->body += this->chr;

    if (this->content_length == 0) {
      // Reached body without ever receiving Content-Length, bail out.
      return REQUEST_ERROR;
    }

    if (this->body.length() == this->content_length) {
      // Reached end of POST body, parse data.
      int ssid_len = this->cur_line.find("&password=") - 5;
      int password_len = (this->cur_line.find("&url=")) - (this->cur_line.find("&password=") + 10);

      std::string ssid = this->cur_line.substr(this->cur_line.find("name=") + 5, ssid_len);
      std::string password = this->cur_line.substr(this->cur_line.find("&password=") + 10, password_len);
      std::string url = this->cur_line.substr(this->cur_line.find("&url=") + 5);

      // Append trailing slash and 'api/data' to url endpoint string.
      if (url.back() != '/') {
        url += '/';
      }
      url += "api/data";

      // Replace + with spaces in ssid string.
      std::replace(ssid.begin(), ssid.end(), '+', ' ');

      // Unescape URL-escaped characters.
      this->ParseURLEncodedData(ssid);
      this->ParseURLEncodedData(password);
      this->ParseURLEncodedData(url);

      // Save values to file and finish.
      ConfigHandler ch;
      ch.WriteConfig(String(ssid.c_str()), String(password.c_str()), String(url.c_str()));
      return REQUEST_FINISHED_POST;
    }
  }
}

/**
 * Handles a single line from a client's request.
 */
unsigned char RequestHandler::HandleIncomingLine() {
  if (this->request_is_get) {
    return this->HandleGET();
  } else if (this->request_is_post) {
    return this->HandlePOST();
  } else {
    // Still reading the request type.
    if (this->cur_line.find("GET") == 0) {
      this->request_is_get = true;
      return REQUEST_ONGOING;
    }

    if (this->cur_line.find("POST") == 0) {
      this->request_is_post = true;
      return REQUEST_ONGOING;
    }
  }
}

/**
 * Handles a single line from a client's GET request.
 */
unsigned char RequestHandler::HandleGET() {
  if (this->cur_line.length() == 0) {
    // End of client headers, time to send a response.
    this->client.println(
        "HTTP/1.1 200 OK\n"
        "Content-type:text/html\n"
        "Connection: close\n");

    // Read index.html and send it to the client.
    File html = SPIFFS.open("/index.html", "r");
    if (html) {
      while (html.available()) {
        client.println(html.readStringUntil('\n'));
      }
      html.close();
    } else {
      // Couldn't open HTML file, bail out.
      return REQUEST_ERROR;
    }

    return REQUEST_FINISHED_GET;
  }

  // Ignore incoming headers.
  return REQUEST_ONGOING;
}

/**
 * Handles a single line from a client's POST request.
 */
unsigned char RequestHandler::HandlePOST() {
  if (!this->reading_request_body) {
    if (this->cur_line.length() == 0) {
      // Empty line after the headers. Body starts here.
      this->reading_request_body = true;
      return REQUEST_ONGOING;
    }

    if (this->cur_line.find("Content-Length: ") == 0) {
      // Found Content-Length header, parse it.
      this->content_length = std::atoi(this->cur_line.substr(16).c_str());
      return REQUEST_ONGOING;
    }
  }
}

/**
 * Un-escapes escaped characters in the given string (ex.: %28 -> `(`)
 */
void RequestHandler::ParseURLEncodedData(std::string &data) {
  size_t pos;

  while ((pos = data.find("+")) != std::string::npos) {
    data.replace(pos, 1, " ");
  }

  while ((pos = data.find("%")) != std::string::npos) {
    if (pos <= data.length() - 3) {
      char replace[2] = {(char)(std::stoi("0x" + data.substr(pos + 1, 2), NULL, 16)), '\0'};
      data.replace(pos, 3, replace);
    }
  }
}
