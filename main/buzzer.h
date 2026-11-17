#pragma once

void buzzerSetup();

// Blocking: plays `beeps` beeps of `durationMs`, separated by `gapMs`.
void playSound(int beeps = 2, int gapMs = 50, int durationMs = 100);
