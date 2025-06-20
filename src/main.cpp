#include <Arduino.h>

#include "../include/config.h"
#include "../include/measurements.h"
#include "../include/web_handlers.h"
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  
  pinMode(SENSOR1_PIN, INPUT_PULLUP);
  pinMode(SENSOR2_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR1_PIN), handleSensor1, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR2_PIN), handleSensor2, FALLING);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed. Formatting...");
  }

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/reset", handleReset);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/distance", HTTP_GET, handleDistance);
  server.on("/style.css", handleCSS);
  server.on("/script.js", handleJS);

  server.begin();

}

void loop() {
  server.handleClient();
  processMeasurements();

  delay(1); // Добавляем небольшую задержку
}
