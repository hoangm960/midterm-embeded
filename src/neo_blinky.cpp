#include "neo_blinky.h"

Adafruit_NeoPixel strip(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

static inline void ledRGB(uint8_t r, uint8_t g, uint8_t b)
{
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}
static inline void ledOff() { ledRGB(0, 0, 0); }

static const uint8_t
    COMF_R = 0,
    COMF_G = 200, COMF_B = 0,                  // Green
    MOIST_R = 255, MOIST_G = 200, MOIST_B = 0, // Yellow
    WET_R = 255, WET_G = 0, WET_B = 0,         // Red
    STALE_R = 150, STALE_G = 0, STALE_B = 150; // Purple

static float getSafeHumidity()
{
    float h = NAN;
    if (xSemaphoreTake(xGlobalDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        h = glob_humidity;
        xSemaphoreGive(xGlobalDataMutex);
    }
    return h;
}

void neo_blinky(void *pvParameters)
{

    strip.begin();
    strip.setBrightness(BRIGHTNESS);
    strip.show();

    Serial.println("=== NeoPixel Humidity Indicator Started ===");

    ledRGB(200, 0, 0);
    vTaskDelay(150);
    ledRGB(0, 200, 0);
    vTaskDelay(150);
    ledRGB(0, 0, 200);
    vTaskDelay(150);
    ledOff();

    bool blink = false;

    for (;;)
    {
        if (xSemaphoreTake(xNewSampleSem, pdMS_TO_TICKS(8000)) != pdTRUE)
        {
            Serial.println("[NEO] STALE → Purple pulse");
            ledRGB(STALE_R, STALE_G, STALE_B);
            vTaskDelay(200);
            ledOff();
            vTaskDelay(800);
            continue;
        }

        float h = getSafeHumidity();

        if (isnan(h))
        {
            Serial.println("[NEO] ERROR → Purple pulse");
            ledRGB(STALE_R, STALE_G, STALE_B);
            vTaskDelay(200);
            ledOff();
        }
        else if (h <= HUMIDITY_MOIST)
        {
            Serial.printf("[NEO] COMFORT (%.2f%%) → Solid Green\n", h);
            ledRGB(COMF_R, COMF_G, COMF_B);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else if (h <= HUMIDITY_WET)
        {
            Serial.printf("[NEO] MOIST (%.2f%%) → Blinking Yellow\n", h);
            blink = !blink;
            if (blink)
                ledRGB(MOIST_R, MOIST_G, MOIST_B);
            else
                ledOff();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else
        {
            Serial.printf("[NEO] WET (%.2f%%) → Fast Red Blink\n", h);
            for (int i = 0; i < 3; ++i)
            {
                ledRGB(WET_R, WET_G, WET_B);
                vTaskDelay(100);
                ledOff();
                vTaskDelay(100);
            }
            vTaskDelay(pdMS_TO_TICKS(600));
        }
    }
}