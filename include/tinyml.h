#ifndef __TINY_ML__
#define __TINY_ML__

#include <Arduino.h>
#include <Ultrasonic.h>
#include "./MLscript/dht_anomaly_model.h"
#include "global.h"

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#define LIGHT_PIN 3
#define TRIGGER_PIN 1
#define ECHO_PIN 2

void setupTinyML();
void tiny_ml_task(void *pvParameters);

#endif