#include "buzzer.h"

void buzz_init() {
  pinMode(P_BUZZ, OUTPUT);
  randomSeed(analogRead(A1));
}

void buzzStartup() {
  if (random(5) == 0) {
    int notes[] = {659, 659, 0,   659, 0,   523, 659, 784, 0,   392,
                   523, 0,   392, 0,   330, 440, 494, 466, 440, 392,
                   659, 784, 831, 698, 784, 0,   659, 523, 587, 494};
    int durs[] = {100, 100, 100, 100, 100, 100, 100, 200, 300, 300,
                  200, 100, 200, 100, 200, 200, 200, 100, 200, 133,
                  133, 133, 200, 100, 100, 100, 200, 200, 200, 400};
    int gaps[] = {30, 30, 0,  30, 0,  30, 30, 30, 0,  0,  30, 0,  30, 0,  30,
                  30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 0,  30, 30, 30, 0};
    for (int i = 0; i < 30; i++) {
      if (notes[i] == 0)
        noTone(P_BUZZ);
      else
        tone(P_BUZZ, notes[i], durs[i]);
      delay(durs[i] + gaps[i]);
    }
  } else {
    tone(P_BUZZ, 880, 100);
    delay(150);
    tone(P_BUZZ, 1175, 200);
    delay(200);
  }
  noTone(P_BUZZ);
}

void buzzResult(ComponentType comp) {
  switch (comp) {
  case RESISTOR:
    // single short beep
    tone(P_BUZZ, 880, 80);
    delay(80);
    noTone(P_BUZZ);
    break;
  case CAPACITOR:
    // two quick beeps
    tone(P_BUZZ, 1046, 60);
    delay(80);
    noTone(P_BUZZ);
    delay(60);
    tone(P_BUZZ, 1046, 60);
    delay(60);
    noTone(P_BUZZ);
    break;
  case OPEN_CIRCUIT:
    // low long beep
    tone(P_BUZZ, 300, 400);
    delay(400);
    noTone(P_BUZZ);
    break;
  default:
    break;
  }
}
