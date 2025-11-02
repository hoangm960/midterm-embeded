#ifndef __LED_BLINKY__
#define __LED_BLINKY__
#include <Wire.h>
#include <DHT20.h>
#include "global.h"

#define LED_PIN 48
#define SDA_PIN 11
#define SCL_PIN 12

void led_blinky(void *pvParameters);


#endif