#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "global.h"

#define NEO_PIN 45
#define LED_COUNT 1 
#define NUM_PIXELS 1
#define BRIGHTNESS 120

void neo_blinky(void *pvParameters);


#endif