#pragma once
#include <Arduino.h>

enum ComponentType {
    OPEN_CIRCUIT,
    RESISTOR,
    CAPACITOR,
    DIODE
};

// initialises outputs for the display
void init_d();

void display(ComponentType comp, int number);
