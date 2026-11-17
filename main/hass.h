#pragma once

// Home Assistant link: WebSocket subscription for live updates, REST for the
// initial state after each (re)connect.

// Drive the connection. Call every loop iteration; idles while WiFi is down.
void hassLoop();
bool hassIsConnected();
// Milliseconds since the link was last ready (0 while connected).
uint32_t hassDisconnectedForMs();
