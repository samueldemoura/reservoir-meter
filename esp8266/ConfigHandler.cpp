#define CONFIG_PATH "/config.ini"

#include "ConfigHandler.h"
#include <Arduino.h>
#include <FS.h>
#include <tuple>

/**
 * Constructor with default values.
 */
ConfigHandler::ConfigHandler() {
  std::get<0>(this->config) = "fakessid";
  std::get<1>(this->config) = "fakepassword";
  std::get<2>(this->config) = "fakeendpoint";
}

/**
 * Read configuration values from file.
 */
void ConfigHandler::ReadConfig() {
  Serial.println(" * Attempting to open config file for reading...");
  File config_file = SPIFFS.open(CONFIG_PATH, "r");

  if (config_file) {
    Serial.println(" * Opened config file. Reading values...");

    std::get<0>(this->config) = String(config_file.readStringUntil('\n'));
    std::get<1>(this->config) = String(config_file.readStringUntil('\n'));
    std::get<2>(this->config) = String(config_file.readStringUntil('\n'));
    std::get<0>(this->config).trim();
    std::get<1>(this->config).trim();
    std::get<2>(this->config).trim();

    config_file.close();
  } else {
    Serial.println(" * Failed to open config file. Using default values...");
  }
}

/**
 * Write configuration values to file.
 */
void ConfigHandler::WriteConfig(String ssid, String password, String endpoint) {
  File config_file = SPIFFS.open(CONFIG_PATH, "w");

  config_file.println(ssid.c_str());
  config_file.println(password.c_str());
  config_file.println(endpoint.c_str());

  config_file.close();
}

/**
 * Getters.
 */
String ConfigHandler::GetSSID() {
  return std::get<0>(this->config);
}

String ConfigHandler::GetPassword() {
  return std::get<1>(this->config);
}

String ConfigHandler::GetEndpoint() {
  return std::get<2>(this->config);
}
