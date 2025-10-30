#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include "global.h"

#define SDA_PIN 11
#define SCL_PIN 12

void temp_humi_monitor(void *pvParameters);


#endif