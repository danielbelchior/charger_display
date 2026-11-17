#include "buzzer.h"
#include "config.h"

void buzzerSetup() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);  // active-low: HIGH is silent
}

void playSound(int beeps, int gapMs, int durationMs) {
    for (int i = 0; i < beeps; i++) {
        digitalWrite(BUZZER_PIN, LOW);
        delay(durationMs);
        digitalWrite(BUZZER_PIN, HIGH);
        if (i < beeps - 1) {
            delay(gapMs);
        }
    }
}
