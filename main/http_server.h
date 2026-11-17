#pragma once

#include <Arduino.h>

// Web UI and REST API on HTTP_PORT, live status push on STATUS_WS_PORT.

// Start the servers. Safe to call repeatedly; only the first call has effect.
void httpServerSetup();
void httpServerLoop();

// Status document shared by GET /status and the status WebSocket.
void buildStatusJson(String& out);
