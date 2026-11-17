#pragma once

#include "wifi_policy.h"

void wifiSetup();
// Drive the connection. Call every loop iteration.
WifiChange wifiLoop();

bool wifiIsConnected();
uint32_t wifiConnectedForMs();
uint32_t wifiDisconnectCount();
const char* wifiLastDisconnectReason();
