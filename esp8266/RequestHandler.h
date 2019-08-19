#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <ESP8266HTTPClient.h>
#include <string>

class RequestHandler {
private:
  WiFiClient client;

  std::string body;
  unsigned char chr;
  unsigned int content_length;
  std::string cur_line;

  bool request_is_get;
  bool request_is_post;
  bool reading_request_body;

  unsigned char HandleGET();
  unsigned char HandlePOST();
  unsigned char HandleIncomingChar();
  unsigned char HandleIncomingLine();
  void ParseURLEncodedData(std::string &data);

public:
  RequestHandler(WiFiClient client);
  unsigned char HandleClient();
};

#endif
