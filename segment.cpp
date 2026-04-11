#include "segment.h"

#define COMPONENT_HOLD_TIME 1000
#define VALUE_HOLD_TIME 2000

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
static const uint8_t _openMap[4] = {0b0111111, 0b1110011, 0b1111001, 0b0110111};
static const uint8_t _resiMap[4] = {0b1010000, 0b1111001, 0b1101101, 0b0010000};
static const uint8_t _capMap[4]  = {0b0111001, 0b1110111, 0b1110011, 0b0000000};
static const uint8_t _dioMap[4]  = {0b0111101, 0b0010000, 0b0111011, 0b0111101};
static const uint8_t _dashMap[4]  = {0b0000001, 0b0000001, 0b0000001, 0b0000001};

void init_d() {
    for (int i = 0; i < 4; i++) {
        pinMode(_digitPins[i], OUTPUT);
        digitalWrite(_digitPins[i], HIGH);
    }

    for (int i = 0; i < 7; i++) {
        pinMode(_segmentPins[i], OUTPUT);
        digitalWrite(_segmentPins[i], LOW);
    }
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
static void _multiplex(const uint8_t patterns[4], unsigned long duration_ms) {
    unsigned long start = millis();
    while (millis() - start < duration_ms) {
        for (int i = 0; i < 4; i++) {
            digitalWrite(_digitPins[i == 0 ? 3 : i - 1], HIGH);
            _allSegmentsOff();
            _setSegments(patterns[i]);
            digitalWrite(_digitPins[i], LOW);
            delayMicroseconds(2500);
        }
        digitalWrite(_digitPins[3], HIGH);
        _allSegmentsOff();
    }
}

void display(ComponentType comp, int number) {
    // show componenet
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
    _multiplex(txtPattern, COMPONENT_HOLD_TIME);

    if(comp == OPEN_CIRCUIT) {
        _multiplex(_dashMap, 2000)
    } else {
        number = abs(number) % 10000;
        uint8_t numPatterns[4] = {
            _digitMap[(number / 1000) % 10],
            _digitMap[(number / 100)  % 10],
            _digitMap[(number / 10)   % 10],
            _digitMap[number % 10]
        };
        _multiplex(numPatterns, VALUE_HOLD_TIME);
    }    
}

const uint8_t* _get_digit_pins() {
    return _digitPins;
}

const uint8_t* _get_segment_pins() {
    return _segmentPins;
}