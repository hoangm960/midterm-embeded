#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Sensor data
extern float glob_temperature;
extern float glob_humidity;

// AP Config
extern String ssid;
extern String password;

extern SemaphoreHandle_t xGlobalDataMutex;
extern SemaphoreHandle_t xNewSampleSem;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

#endif