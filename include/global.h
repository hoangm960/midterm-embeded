#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


extern float glob_temperature;
extern float glob_humidity;

extern String ssid;
extern String password;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
#endif