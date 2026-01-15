// config.cpp
#include <cstring>

#include "config.h"
#include <Preferences.h>

Preferences preferences;

// Определения с инициализацией
char ssid[32] = "MG";
char password[64] = "25031991";
const int serverPort = 80;

void loadWiFiSettings() {
  preferences.begin("wifi-config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPass = preferences.getString("password", "");
  
  if (!savedSSID.isEmpty()) {
    strncpy(ssid, savedSSID.c_str(), sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';  // Гарантируем терминатор
  }
  if (!savedPass.isEmpty()) {
    strncpy(password, savedPass.c_str(), sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';  // Гарантируем терминатор
  }
  preferences.end();
}

void saveWiFiSettings() {
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}