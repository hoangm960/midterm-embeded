#ifndef ___MAIN_SERVER__
#define ___MAIN_SERVER__
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "global.h"

#define LED1_PIN 48
#define LED2_PIN 41
#define BOOT_PIN 0
// extern WebServer server;

// extern bool isAPMode;

String mainPage();

void startAP();
void setupServer();

void main_server_task(void *pvParameters);

#endif