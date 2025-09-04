#ifndef LOGGING_H
#define LOGGING_H

#include "declarations.h"

void logInfo(String message);
void logWarning(String message);
void logError(String message);
void logDebug(String message);
void _logMessage(String message);

#endif // LOGGING_H
