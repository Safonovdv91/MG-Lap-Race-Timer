// web_handlers.cpp
#include "web_handlers.h"
#include <WebServer.h>
#include "measurements.h"

WebServer server(serverPort);

extern Mode currentMode;
extern float distance;

void handleRoot();
void handleData();
void handleReset();
void handleMode();
void handleDistance();
void handleCSS();
void handleJS();


void handleRoot() {
  String html = getHTMLContent();
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"mode\":" + String(currentMode) + ",";
  json += "\"currentValue\":" + String(currentValue) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"measurementInProgress\":" + String(sensor1Triggered ? "true" : "false") + ",";
  json += "\"historyIndex\":" + String(historyIndex) + ",";
  
  json += "\"speedHistory\":[";
  for (int i = 0; i < min(historyIndex, HISTORY_SIZE); i++) {
    if (i > 0) json += ",";
    json += "{\"value\":" + String(speedHistory[i].value) + ",\"time\":" + String(speedHistory[i].timestamp) + "}";
  }
  json += "],";
  
  json += "\"lapHistory\":[";
  for (int i = 0; i < min(historyIndex, HISTORY_SIZE); i++) {
    if (i > 0) json += ",";
    json += "{\"value\":" + String(lapHistory[i].value) + ",\"time\":" + String(lapHistory[i].timestamp) + "}";
  }
  json += "]";
  
  json += "}";
  server.send(200, "application/json", json);
}

void handleReset() {
  startTime = 0;
  endTime = 0;
  sensor1Triggered = false;
  sensor2Triggered = false;
  currentValue = 0.0;
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  if (server.hasArg("value")) {
    currentMode = (Mode)server.arg("value").toInt();
    resetMeasurements();
  }
  server.send(200, "text/plain", "OK");
}

void handleDistance() {
  if (server.hasArg("value")) {
    distance = server.arg("value").toFloat();
  }
  server.send(200, "text/plain", "OK");
}

void handleCSS() {
  if (SPIFFS.exists("/style.css")) {
    File file = SPIFFS.open("/style.css", "r");
    if (file) {
      server.streamFile(file, "text/css");
      file.close();
      return;
    }
  }
  server.send(404, "text/plain", "CSS not found");
}

void handleJS() {
  if (SPIFFS.exists("/script.js")) {
    File file = SPIFFS.open("/script.js", "r");
    if (file) {
      server.streamFile(file, "application/javascript");
      file.close();
      return;
    }
  }
  server.send(404, "text/plain","JS not found");
}

void resetMeasurements() {
  startTime = 0;
  endTime = 0;
  sensor1Triggered = false;
  sensor2Triggered = false;
  measurementReady = false;
  currentValue = 0.0;
}
