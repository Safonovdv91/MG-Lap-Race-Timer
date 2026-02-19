#include "websocket_handlers.h"
#include "measurements.h"
#include "config.h"
#include "web_handlers.h" // To get transmitter data
#include <ArduinoJson.h>
#include "web_content.h"
#include "battery/battery.h"

WebSocketsServer webSocket = WebSocketsServer(81);

// Use a global JSON document to avoid heap fragmentation
StaticJsonDocument<1024> ws_doc;

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      // Send current state immediately to new client
      ws_broadcast_data();
      break;
    }
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      // We can handle incoming messages here if needed in the future
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void ws_init() {
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println("WebSocket server started at port 81");
}

void ws_loop() {
  webSocket.loop();
}

void ws_broadcast_data() {
  ws_doc.clear();

  ws_doc["value"] = currentValue;
  ws_doc["battery"] = getBatteryPercentage();
  ws_doc["voltage"] = getBatteryVoltage();

  TimerStatus status = getTimerStatus();
  switch(status) {
    case STATUS_READY:
      ws_doc["timer_status"] = "ready";
      break;
    case STATUS_RUNNING:
      ws_doc["timer_status"] = "running";
      break;
    case STATUS_DISPLAY:
      ws_doc["timer_status"] = "display";
      break;
  }

  #ifdef RECEIVER_MODE
  int txBatteryLevel = getTransmitterBatteryLevel();
  ws_doc["tx_battery"] = txBatteryLevel;
  ws_doc["tx_voltage"] = getTransmitterBatteryVoltage();
  #endif

  JsonArray history = ws_doc.createNestedArray("history");
  
  lockMeasurements();
  Measurement* history_source = lapHistory;
  for(int i = 0; i < HISTORY_SIZE && i < historyIndex; i++) {
    if(history_source[i].value > 0) {
      JsonObject histObj = history.createNestedObject();
      histObj["value"] = history_source[i].value;
      formatAndSetTimestamp(histObj, "timestamp", history_source[i].timestamp);
    }
  }
  unlockMeasurements();

  if (getMeasurementInProgressSafe() || status == STATUS_DISPLAY) {
      ws_doc["race_time"] = getCurrentRaceTimeSafe() / 1000000.0;
  } else {
      ws_doc["race_time"] = 0;
  }

  // Serialize to a static buffer to avoid heap fragmentation from String objects
  char buffer[1024];
  size_t len = serializeJson(ws_doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}
