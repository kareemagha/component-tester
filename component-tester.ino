#include "segment.h"
#include <math.h>

#define P_OUT 0
#define P_READ A0
#define VCC 5.0
#define SAMPLE_CNT 10.0f
#define UP_LIM 1000
#define LW_LIM 20
#define REF_R 10000
#define ADC_TAU     647
#define TIMEOUT_US  2000000UL
#define MIN_CAP_US  50

ComponentType detectComponent(float &val);
float measureResistance();
float measureCapacitance();

void setup() {
  pinMode(P_OUT, OUTPUT);
  pinMode(P_READ, INPUT);
  init_d();
}

void loop() {
  float val;
  ComponentType comp = detectComponent(val);
  display(comp, (int)val);
}

// measure R
// measure C
// if C is very very low, mos tlikely its R
// else its C

ComponentType detectComponent(float &val) {
    val = 0;

    digitalWrite(P_OUT, HIGH);
    delayMicroseconds(200);

    int adcValue = analogRead(P_READ);
    digitalWrite(P_OUT, LOW);

    // upper limit
    // 0.95* 1024 = 980

    if (adcValue > UP_LIM) {
      return OPEN_CIRCUIT;
    }

    float cap = measureCapacitance();
    if (cap > 0.01f) {
       // converts from uF to nF
        val = cap * 1000.0f;
        return CAPACITOR;
    }

    val = measureResistance();
    return RESISTOR;
}

float measureResistance() {
    digitalWrite(P_OUT, HIGH);
    delayMicroseconds(500);

    // take an average of 10 samples
    long sum = 0;
    for (int i = 0; i < (int)SAMPLE_CNT; i++) {
        sum += analogRead(P_READ);
        delayMicroseconds(50);
    }
    float adc = sum / SAMPLE_CNT;

    digitalWrite(P_OUT, LOW);

    if (adc > UP_LIM || adc < LW_LIM) {
      return 0;
    }

    float v = (adc / 1023.0f) * VCC;
    return REF_R * (v) / (VCC - v);
}

float measureCapacitance() {
    digitalWrite(P_OUT, LOW);

    // current time
    unsigned long time = micros();
    while (analogRead(P_READ) > 10) {
        if (micros() - time > TIMEOUT_US) return 0.0f;
    }
    delay(2);

    unsigned long t0 = micros();
    digitalWrite(P_OUT, HIGH);

    bool done = false;
    while (micros() - t0 < TIMEOUT_US) {
        if (analogRead(P_READ) >= ADC_TAU) {
            done = true;
            break;
        }
    }

    unsigned long elapsed = micros() - t0;
    digitalWrite(P_OUT, LOW);

    if (done == false || elapsed < (unsigned long)MIN_CAP_US)
        return 0.0f;

    return (float)elapsed / REF_R;
