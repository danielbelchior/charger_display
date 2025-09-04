#ifndef HASS_H
#define HASS_H

#include "declarations.h"

// --- Function Prototypes ---

// Setup
void setupHass();

// Core Loop
void loopHass();

// Functions
String getEntityState(String entityId);
void onMessage(WebsocketsMessage message);
void onEvent(WebsocketsEvent event, String data);

#endif // HASS_H
