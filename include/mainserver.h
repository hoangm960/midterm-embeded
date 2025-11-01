#ifndef ___MAIN_SERVER__
#define ___MAIN_SERVER__
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include "global.h"

#define FAN_PIN 6
#define EXIT_PIN 4

String mainPage();

void startAP();
void setupServer();

void main_server_task(void *pvParameters);

#endif