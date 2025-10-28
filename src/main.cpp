#include <Arduino.h>
#include <Wire.h>
#include <DHT20.h>

// ---------------- Pin / HW ----------------
#define LED_PIN 48          // On-board (GPIO_NUM_48)
#define SDA_PIN 11          // Yolo UNO I2C
#define SCL_PIN 12

// --------- Temperature thresholds ---------
#define TEMP_COOL_MAX 30   // < 30 -> slow single blink
#define TEMP_WARM_MAX 32  // 30-32 -> double blink
                             // > 32 -> triple blink


static SemaphoreHandle_t xNewSampleSem = NULL;   // binary - new temp ready
static SemaphoreHandle_t xTempMutex    = NULL;   // protect shared var


static float lastTemperature = NAN;


static void setTemp(float t)
{
  if (!xTempMutex) return;
  xSemaphoreTake(xTempMutex, portMAX_DELAY);
  lastTemperature = t;
  xSemaphoreGive(xTempMutex);
}

static float getTemp()
{
  float t = NAN;
  if (xTempMutex) {
    xSemaphoreTake(xTempMutex, portMAX_DELAY);
    t = lastTemperature;
    xSemaphoreGive(xTempMutex);
  }
  return t;
}

// task
void TaskReadSensor(void *pvParameters)
{
  static DHT20 dht20;
  static bool inited = false;

  if (!inited) {
    dht20.begin();               
    inited = true;
  }

  for (;;) {
    if (dht20.read() == 0) {      // successful read
      float t = dht20.getTemperature();

      setTemp(t);                // <-- protected write
      if (xNewSampleSem) xSemaphoreGive(xNewSampleSem);   // signal LED 

      Serial.printf("[SENSOR] Temperature: %.2f °C\n", t);
    } else {
      setTemp(NAN);              // mark error
      if (xNewSampleSem) xSemaphoreGive(xNewSampleSem);
    }

    vTaskDelay(pdMS_TO_TICKS(3000));   // every 3 s
  }
}

void TaskLEDControl(void *pvParameters)
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("=== ESP32-S3 LED Blink with Temperature Conditions ===");

  for (;;) {
    // a new temperature sample (timeout 800 ms) 
    if (xSemaphoreTake(xNewSampleSem, pdMS_TO_TICKS(800)) != pdTRUE) {
      // Timeout -> treat as stale / error
      Serial.println("[LED] STALE -> long ON");
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(300));
      continue;
    }

    float t = getTemp();          // <-- protected read

    if (isnan(t)) {
      // Sensor error -> long ON
      Serial.println("[LED] ERROR -> long ON");
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(300));
    }
    else if (t < TEMP_COOL_MAX) {
      // COOL -> slow single blink
      Serial.printf("[LED] COOL (%.2f°C) -> slow blink\n", t);
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(800));
    }
    else if (t < TEMP_WARM_MAX) {
      // WARM -> double blink
      Serial.printf("[LED] WARM (%.2f°C) -> double blink\n", t);
      for (int i = 0; i < 2; ++i) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(150));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(150));
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    else {
      // HOT -> triple blink
      Serial.printf("[LED] HOT (%.2f°C) -> triple blink\n", t);
      for (int i = 0; i < 3; ++i) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(120));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(120));
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

// set up
void setup()
{
  Serial.begin(115200);
  delay(500);

 
  Wire.begin(SDA_PIN, SCL_PIN);


  xNewSampleSem = xSemaphoreCreateBinary();   // binary semaphore
  xTempMutex    = xSemaphoreCreateMutex();    // mutex for shared temperature

  
  xTaskCreate(TaskLEDControl, "LEDControl", 6144, NULL, 2, NULL);
  xTaskCreate(TaskReadSensor, "ReadSensor", 4096, NULL, 2, NULL);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}