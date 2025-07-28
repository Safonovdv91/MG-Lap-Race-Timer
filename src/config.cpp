// config.cpp
#include <cstring>

#include "config.h"
#include <Preferences.h>

Preferences preferences;

// Определения с инициализацией
const char* ssid = "MG";
const char* password = "25031991";
const int serverPort = 80;

void loadWiFiSettings() {
  preferences.begin("wifi-config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPass = preferences.getString("password", "");
  
  if (!savedSSID.isEmpty()) {
    ssid = strdup(savedSSID.c_str());
  }
  if (!savedPass.isEmpty()) {
    password = strdup(savedPass.c_str());
  }
  preferences.end();
}

void saveWiFiSettings() {
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}