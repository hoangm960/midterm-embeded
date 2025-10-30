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

// FIX: Mutex for thread-safe access to global temp/humi
extern SemaphoreHandle_t xGlobalDataMutex;

// FIX: Semaphore to signal LED task that a new sample is ready
extern SemaphoreHandle_t xNewSampleSem;

// (Other globals like xBinarySemaphoreInternet can remain if needed)
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern boolean isWifiConnected;
extern String wifi_ssid;
extern String wifi_password;

#endif