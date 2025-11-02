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

void led_blinky(void *pvParameters)
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    Serial.println("=== LED Control Task Started ===");

    for (;;)
    {
        if (xSemaphoreTake(xNewSampleSem, pdMS_TO_TICKS(8000)) != pdTRUE)
        {
            Serial.println("[LED] STALE data -> long ON");
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(1000));
            digitalWrite(LED_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(300));
            continue;
        }

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