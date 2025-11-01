#include "global.h"
float glob_temperature = 0;
float glob_humidity = 0;

String ssid = "UIAAAAAA!!!";
String password = "11111111";

SemaphoreHandle_t xGlobalDataMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t xNewSampleSem = xSemaphoreCreateBinary();
