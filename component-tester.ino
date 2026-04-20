#include "segment.h"
#include "buzzer.h"
#include <math.h>

#define P_OUT 0
#define P_READ A0
#define VCC 5.0
#define SAMPLE_CNT 10.0f
#define UP_LIM 1000
#define LW_LIM 20
#define REF_R 10000
#define ADC_TAU 647
#define TIMEOUT_US 2000000UL
#define MIN_CAP_US 50
#define MIN_CAP_VAL 0.00025f

// ln(1 - 1.1/5.0) = 0.2485
#define BANDGAP_K 0.2485f
#define MIN_TICKS 8

uint16_t baselineV = 0;

ComponentType detectComponent(float &val);
float measureResistance();
float measureCapacitance();
float measureSmallCapacitance();
uint16_t measureRawTicks();

void setup() {
  pinMode(P_OUT, OUTPUT);
  pinMode(P_READ, INPUT);
  buzz_init();
  init_d();

  // get avg
  uint32_t sum = 0;
  uint8_t good = 0;
  for (int i = 0; i < 5; i++) {
    uint16_t t = measureRawTicks();
    if (t > 0) {
      sum += t;
      good++;
    }
  }
  baselineV = good > 0 ? (sum / good) : 0;

  buzzStartup();
}

void loop() {
  float val;
  ComponentType comp = detectComponent(val);
  // noInterrupts();
  display(comp, val);
  buzzResult(comp);
  // interrupts();
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
    // could be open circuit OR a small cap that charged fully in 200us.
    // we can discharge and check.
    // a small cap drains fast, open circuit stays high.

    // discharge (possible) cap before retesting
    digitalWrite(P_OUT, LOW);

    pinMode(P_READ, OUTPUT);
    digitalWrite(P_READ, LOW);

    delayMicroseconds(100);
    pinMode(P_READ, INPUT);

    float smallCap = measureSmallCapacitance();
    if (smallCap > MIN_CAP_VAL) {
      val = smallCap * 1000.0f;
      return CAPACITOR;
    }

    return OPEN_CIRCUIT;
  }

  float cap = measureCapacitance();
  // noInterrupts();
  if (cap > 0.01f) {
    val = cap * 1000.0f;
    // interrupts();
    return CAPACITOR;
  }

  float smallCap = measureSmallCapacitance();
  if (smallCap > 0.0f) {
    digitalWrite(P_OUT, HIGH);
    delay(1);
    int v = analogRead(P_READ);
    digitalWrite(P_OUT, LOW);
    if (v < LW_LIM || v > UP_LIM) {
      val = smallCap * 1000.0f;
      // interrupts();
      return CAPACITOR;
    }
  }

  // interrupts();
  val = measureResistance();
  return RESISTOR;
}

float measureResistance() {
  digitalWrite(P_OUT, HIGH);
  delay(10);

  // apparently gets rid of the noise????????
  for (int i = 0; i < 3; i++)
    analogRead(P_READ);

  // take an average of 10 samples
  // 10-sample averageer lp filter
  long sum = 0;
  for (int i = 0; i < SAMPLE_CNT; i++) {
    sum += analogRead(P_READ);
    delayMicroseconds(100);
  }
  float adc = sum / SAMPLE_CNT;

  digitalWrite(P_OUT, LOW);

  // most likely an OC
  if (adc > UP_LIM || adc < LW_LIM) {
    return 0;
  }

  float v = (adc / 1023.0f) * VCC;
  return REF_R * (v) / (VCC - v);
}

float measureCapacitance() {
  digitalWrite(P_OUT, LOW);

  // allow cap to discharge
  unsigned long time = micros();
  while (analogRead(P_READ) > 10) {
    if (micros() - time > TIMEOUT_US)
      return 0.0f;
  }
  delay(2);

  unsigned long t0 = micros();

  bool done = false;
  digitalWrite(P_OUT, HIGH);

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

  // adjust analogue read latency
  if (elapsed > 104)
    elapsed -= 104;

  return (float)elapsed / REF_R;
}

// basically magic
// uses bandgap refernce voltage
// aswell as mem-mapped IO for faster reading
uint16_t measureRawTicks() {
  // discharge fully
  digitalWrite(P_OUT, LOW);
  unsigned long t = micros();
  while (analogRead(P_READ) > 5) {
    if (micros() - t > TIMEOUT_US)
      return 0;
  }
  delay(2);

  // disable ADC, then route A0 through the ADC mux to comparator AIN-
  uint8_t adcsra_saved = ADCSRA;
  ADCSRA &= ~(1 << ADEN);
  ADMUX = (ADMUX & 0xF0) | 0x00;
  ADCSRB |= (1 << ACME);

  // enable comparator
  // bandgap (1.1V) on AIN+, A0 on AIN-, wired to timer1
  // capture
  ACSR = (1 << ACBG) | (1 << ACIC) | (1 << ACI);

  // 70us rest
  delayMicroseconds(80);

  // save configs for later
  uint8_t tccr1a_saved = TCCR1A;
  uint8_t tccr1b_saved = TCCR1B;

  // clear timer1s control register A
  TCCR1A = 0;
  // rmove prescaler allowing 62.5ns per tick 1/16M
  TCCR1B = (1 << CS10);

  // reset counter
  TCNT1 = 0;
  // clear flags
  TIFR1 = (1 << ICF1);

  // start charging - set P0 to high
  PORTD |= (1 << PD0);

  // ICF1 goes high when A0 goes above 1.1V
  unsigned long start = micros();
  while (!(TIFR1 & (1 << ICF1))) {
    // timeout after 100ms
    if (micros() - start > 100000UL) {
      PORTD &= ~(1 << PD0);
      ACSR = 0;
      ADCSRB &= ~(1 << ACME);
      ADCSRA = adcsra_saved;
      TCCR1A = tccr1a_saved;
      TCCR1B = tccr1b_saved;
      return 0;
    }
  }

  // tick count when ICF1 goes high
  uint16_t captured = ICR1;

  PORTD &= ~(1 << PD0);
  ACSR = 0;
  ADCSRB &= ~(1 << ACME);
  ADCSRA = adcsra_saved;
  TCCR1A = tccr1a_saved;
  TCCR1B = tccr1b_saved;

  return captured;
}

float measureSmallCapacitance() {
  uint16_t ticks = measureRawTicks();

  if (ticks < MIN_TICKS)
    return 0.0f;

  if (ticks > baselineV)
    ticks -= baselineV;
  else
    return 0.0f;

    // 16M / 16 = 1M -> 1us
  float elapsed_us = ticks / 16.0f;
  return elapsed_us / (REF_R * BANDGAP_K);
}
