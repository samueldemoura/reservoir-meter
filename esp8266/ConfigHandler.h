#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <Arduino.h>
#include <tuple>

class ConfigHandler {
private:
  std::tuple<String, String, String> config;

public:
  ConfigHandler();

  void ReadConfig();
  void WriteConfig(String ssid, String password, String endpoint);

  String GetSSID();
  String GetPassword();
  String GetEndpoint();
};

#endif
