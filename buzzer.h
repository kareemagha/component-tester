#pragma once
#include "segment.h"
#include <Arduino.h>


#define P_BUZZ A3

void buzz_init();
void buzzStartup();
void buzzResult(ComponentType comp);
