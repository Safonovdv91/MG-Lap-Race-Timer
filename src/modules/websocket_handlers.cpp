#include <ArduinoJson.h>

#include "modules/websocket_handlers.h"
#include "core/measurement_core.h"
#include "utils/config.h"
#include "modules/web_handlers.h"
#include "modules/web_content.h"
#include "drivers/battery/battery.h"

#ifdef RECEIVER_MODE
#include "modules/transmitter_data.h"
#endif

// Внешняя переменная из measurement_core.cpp
extern Mode currentMode;

WebSocketsServer webSocket = WebSocketsServer(81);

// Расчёт: базовые поля (~100 байт) + история (5 × ~50 байт) + оверхед 50% = ~600 байт
// Используем 1200 байт для запаса
StaticJsonDocument<1200> ws_doc;

TaskHandle_t wsTaskHandle = NULL;
volatile bool g_sendInitialState = false;

static bool clientQuarantined[WEBSOCKETS_SERVER_CLIENT_MAX] = {false};
static unsigned long quarantineUntil[WEBSOCKETS_SERVER_CLIENT_MAX] = {0};
const unsigned long QUARANTINE_RETRY_MS = 15000; // повторная попытка раз в 15с

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        clientQuarantined[num] = false;
        g_sendInitialState = true;   // просто ставим флаг, ничего не шлём здесь
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

  ws_doc["value"] = getCurrentValueSafe();
  ws_doc["battery"] = getBatteryPercentage();
  ws_doc["voltage"] = getBatteryVoltage();
  
  // Отправляем текущий режим для обновления UI
  ws_doc["mode"] = currentMode;

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

  char buffer[1024];
  size_t len = serializeJson(ws_doc, buffer);

  // БЕЗОПАСНЫЙ ЦИКЛ С ДЕТЕКТОРОМ БЛОКИРОВКИ
  for (uint8_t i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
      if (!webSocket.clientIsConnected(i)) continue;

      // клиент в карантине и время ещё не вышло — вообще не трогаем сокет
      if (clientQuarantined[i] && millis() < quarantineUntil[i]) {
          continue;
      }

      unsigned long start = micros();
      bool success = webSocket.sendTXT(i, buffer, len);
      unsigned long elapsed = micros() - start;

      if (elapsed > 100000 || !success) {
          Serial.printf("[WS] Клиент %d недоступен (задержка %lu мкс). Карантин на %lus.\n",
                        i, elapsed, QUARANTINE_RETRY_MS / 1000);
          clientQuarantined[i] = true;
          quarantineUntil[i] = millis() + QUARANTINE_RETRY_MS;
          webSocket.disconnect(i);
          yield();
      } else {
          clientQuarantined[i] = false; // успешно отправили — снимаем карантин
      }
  }
}

void wsTask(void* pvParameters) {
    const TickType_t broadcastInterval = pdMS_TO_TICKS(90);
    TickType_t lastBroadcast = xTaskGetTickCount();

    for (;;) {
        ws_loop(); // тут внутри webSocket.loop(), onEvent может сработать и просто выставить флаг

        bool doBroadcast = (xTaskGetTickCount() - lastBroadcast >= broadcastInterval);

        if (doBroadcast || g_sendInitialState) {
            g_sendInitialState = false;
            ws_broadcast_data(); // теперь вызывается СНАРУЖИ webSocket.loop(), не реентрантно
            lastBroadcast = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void ws_start_task() {
    xTaskCreatePinnedToCore(
        wsTask,
        "wsTask",
        8192,          // стек побольше — JsonDocument+буфер на 1024 байта внутри
        NULL,
        1,             // приоритет
        &wsTaskHandle,
        0              // core 0 — Arduino loop() обычно живёт на core 1
    );
}