#include "util.h"
#include "declarations.h"
#include <Arduino.h>

void playSound(int beeps, int delayBetweenBeep, int duration) {
    // Play 'beeps' short beeps for any state change.
    for (int i = 0; i < beeps; i++) {
        digitalWrite(BUZZER_PIN, LOW);
        delay(duration);
        digitalWrite(BUZZER_PIN, HIGH);
        if (i < beeps - 1) {
            delay(delayBetweenBeep);
        }
    }
}
