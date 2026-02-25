// config.cpp
#include <cstring>

#include "config.h"
#include <Preferences.h>

Preferences preferences;

// Определения с инициализацией
#ifdef TRANSMITTER_MODE
  char ssid[32] = "MgTimer-01(TX)";
  char password[64] = "25031991";
#else
  char ssid[32] = "MGTimer-01";
  char password[64] = "25031991";
#endif
const int serverPort = 80;

void loadWiFiSettings() {
  preferences.begin("wifi-config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPass = preferences.getString("password", "");
  
  if (!savedSSID.isEmpty()) {
    strncpy(ssid, savedSSID.c_str(), sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
  }
  if (!savedPass.isEmpty()) {
    strncpy(password, savedPass.c_str(), sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';
  }
  preferences.end();
}

void saveWiFiSettings() {
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}