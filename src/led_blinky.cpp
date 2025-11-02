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

void blink(int onTime, int offTime, int times = 1)
{
    for (int i = 0; i < times; ++i)
    {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(onTime));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(offTime));
    }
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
            blink(1000, 300);
            continue;
        }

        float t = getTemp();

        if (isnan(t))
        {
            Serial.println("[LED] ERROR (NaN) -> long ON");
            blink(1000, 300);
        }
        else if (t < TEMP_WARN)
        {
            Serial.printf("[LED] COOL (%.2f°C) -> slow blink\n", t);
            blink(200, 800);
        }
        else if (t < TEMP_CRITICAL)
        {
            Serial.printf("[LED] WARM (%.2f°C) -> double blink\n", t);
            blink(150, 150, 2);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else
        {
            Serial.printf("[LED] HOT (%.2f°C) -> triple blink\n", t);
            blink(150, 150, 3);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}