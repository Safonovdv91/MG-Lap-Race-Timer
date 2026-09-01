#ifndef WEBSOCKET_HANDLERS_H
#define WEBSOCKET_HANDLERS_H

#include <WebSocketsServer.h>

void ws_init();
void ws_loop();
void ws_broadcast_data();
void ws_start_task();

#endif
