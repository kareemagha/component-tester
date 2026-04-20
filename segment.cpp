#include "segment.h"

#define COMPONENT_HOLD_TIME 1000
#define VALUE_HOLD_TIME 2000
#define DP_PIN 13

// pins for digits MS-LS
static const uint8_t _digitPins[4] = {2, 3, 9, 10};

// a, b, c, d, e, f, g
static const uint8_t _segmentPins[7] = {5, 4, 11, 12, 8, 6, 7};

// 0..9
static const uint8_t _digitMap[10] = {
    0b1111110,
    0b0110000,
    0b1101101,
    0b1111001,
    0b0110011,
    0b1011011,
    0b1011111,
    0b1110000,
    0b1111111,
    0b1111011
};

// hackjy way to display text
static const uint8_t _openMap[4] = {0b1111110, 0b1100111, 0b1001111, 0b0010101};
static const uint8_t _resiMap[4] = {0b0000101, 0b1001111, 0b1011011, 0b0010000};
static const uint8_t _capMap[4]  = {0b1001110, 0b1110111, 0b1100111, 0b0000000};
static const uint8_t _dioMap[4]  = {0b0111101, 0b0010000, 0b0011101, 0b0111101};
static const uint8_t _dashMap[4]  = {0b0000001, 0b0000001, 0b0000001, 0b0000001};

// unit labels
static const uint8_t _nfMap[4]   = {0b0000000, 0b0000000, 0b0010101, 0b1000111};
static const uint8_t _ohmMap[4]  = {0b0000000, 0b1111110, 0b0010111, 0b0000000};
static const uint8_t _kohmMap[4] = {0b0010111, 0b1111110, 0b0010111, 0b0000000};

void init_d() {
    for (int i = 0; i < 4; i++) {
        pinMode(_digitPins[i], OUTPUT);
        digitalWrite(_digitPins[i], HIGH);
    }

    for (int i = 0; i < 7; i++) {
        pinMode(_segmentPins[i], OUTPUT);
        digitalWrite(_segmentPins[i], LOW);
    }

    pinMode(DP_PIN, OUTPUT);
    digitalWrite(DP_PIN, LOW);
}

static void _allSegmentsOff() {
    for (int i = 0; i < 7; i++)
        digitalWrite(_segmentPins[i], LOW);
}

// set segments given a pattern
static void _setSegments(uint8_t pattern) {
    for (int i = 0; i < 7; i++) {
        bool segOn = (pattern >> (6 - i)) & 1;
        digitalWrite(_segmentPins[i], segOn ? HIGH : LOW);
    }
}

// holds display for some duration
static void _multiplex(const uint8_t patterns[4], unsigned long duration_ms, int8_t dpDigit = -1) {
    unsigned long start = millis();
    while (millis() - start < duration_ms) {
        for (int i = 0; i < 4; i++) {
            digitalWrite(_digitPins[i == 0 ? 3 : i - 1], HIGH);
            _allSegmentsOff();
            digitalWrite(DP_PIN, LOW);
            _setSegments(patterns[i]);
            if (i == dpDigit) digitalWrite(DP_PIN, HIGH);
            digitalWrite(_digitPins[i], LOW);
            delayMicroseconds(2500);
        }
        digitalWrite(_digitPins[3], HIGH);
        _allSegmentsOff();
        digitalWrite(DP_PIN, LOW);
    }
}

void display(ComponentType comp, float value) {
    // show component name
    const uint8_t* text;
    switch (comp) {
        case OPEN_CIRCUIT:
            text = _openMap;
            break;
        case RESISTOR:
            text =_resiMap;
            break;
        case CAPACITOR:
            text = _capMap;
            break;
        case DIODE:
            text = _dioMap;
            break;
        default:
            text = _openMap;
            break;
    }
    _multiplex(text, COMPONENT_HOLD_TIME);

    if(comp == OPEN_CIRCUIT) {
        _multiplex(_dashMap, VALUE_HOLD_TIME);
        return;
    }

    // determine display number and decimal point position
    int displayNum;
    int8_t dpDigit = -1;
    const uint8_t* unitMap = _dashMap;

    if (comp == CAPACITOR) {
        // value is in nF
        unitMap = _nfMap;
        if (value < 10.0f) {
            displayNum = (int)(value * 1000.0f + 0.5f);
            dpDigit = 0;
        } else if (value < 100.0f) {
            displayNum = (int)(value * 100.0f + 0.5f);
            dpDigit = 1;
        } else if (value < 1000.0f) {
            displayNum = (int)(value * 10.0f + 0.5f);
            dpDigit = 2;
        } else {
            displayNum = (int)(value + 0.5f);
        }
    } else if (comp == RESISTOR) {
        // value is in ohms
        if (value >= 10000.0f) {
            unitMap = _kohmMap;
            float kval = value / 1000.0f;
            if (kval < 10.0f) {
                displayNum = (int)(kval * 1000.0f + 0.5f);
                dpDigit = 0;
            } else if (kval < 100.0f) {
                displayNum = (int)(kval * 100.0f + 0.5f);
                dpDigit = 1;
            } else if (kval < 1000.0f) {
                displayNum = (int)(kval * 10.0f + 0.5f);
                dpDigit = 2;
            } else {
                displayNum = (int)(kval + 0.5f);
            }
        } else {
            unitMap = _ohmMap;
            if (value < 10.0f) {
                displayNum = (int)(value * 1000.0f + 0.5f);
                dpDigit = 0;
            } else if (value < 100.0f) {
                displayNum = (int)(value * 100.0f + 0.5f);
                dpDigit = 1;
            } else if (value < 1000.0f) {
                displayNum = (int)(value * 10.0f + 0.5f);
                dpDigit = 2;
            } else {
                displayNum = (int)(value + 0.5f);
            }
        }
    } else {
        displayNum = (int)(value + 0.5f);
    }

    if (displayNum > 9999) displayNum = 9999;
    if (displayNum < 0) displayNum = 0;

    uint8_t numPatterns[4] = {
        _digitMap[(displayNum / 1000) % 10],
        _digitMap[(displayNum / 100)  % 10],
        _digitMap[(displayNum / 10)   % 10],
        _digitMap[displayNum % 10]
    };
    _multiplex(numPatterns, VALUE_HOLD_TIME, dpDigit);

    // show units
    _multiplex(unitMap, COMPONENT_HOLD_TIME);
}
