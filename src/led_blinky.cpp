#include "led_blinky.h"

static float getTemp()
{
  float t = NAN;
  if (xSemaphoreTake(xGlobalDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
  {
    t = glob_temperature;
    xSemaphoreGive(xGlobalDataMutex);
  }
  return t;
}

void TaskLEDControl(void *pvParameters)
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("=== LED Control Task Started ===");

  for (;;)
  {
    // Wait for a signal from the sensor task
    // Timeout after 800ms, longer than the sensor read (3000ms) is not needed
    if (xSemaphoreTake(xNewSampleSem, pdMS_TO_TICKS(8000)) != pdTRUE)
    {
      // This will happen if the sensor task crashes
      Serial.println("[LED] STALE data -> long ON");
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(300));
      continue;
    }

    // We got a signal, get the temp safely
    float t = getTemp();

    if (isnan(t))
    {
      Serial.println("[LED] ERROR (NaN) -> long ON");
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(300));
    }
    else if (t < TEMP_COOL_MAX)
    {
      Serial.printf("[LED] COOL (%.2f°C) -> slow blink\n", t);
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(800));
    }
    else if (t < TEMP_WARM_MAX)
    {
      Serial.printf("[LED] WARM (%.2f°C) -> double blink\n", t);
      for (int i = 0; i < 2; ++i)
      {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(150));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(150));
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    else
    {
      Serial.printf("[LED] HOT (%.2f°C) -> triple blink\n", t);
      for (int i = 0; i < 3; ++i)
      {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(120));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(120));
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

void led_blinky(void *pvParameters)
{
  xTaskCreate(TaskLEDControl, "LEDControl", 4096, NULL, 2, NULL);
  vTaskDelete(NULL);
}