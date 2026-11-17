#pragma once

// Arduino OTA updates, compiled in only when ENABLE_OTA is 1.

// Call once the network is up.
void otaSetup();
// Returns true while an update is being received; skip other work then.
bool otaLoop();
