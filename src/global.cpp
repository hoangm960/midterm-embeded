#include "global.h"
float glob_temperature = 0;
float glob_humidity = 0;

String ssid = "Data here!!!!";
String password = "11111111";
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();