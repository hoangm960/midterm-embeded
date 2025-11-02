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

void blink(int R, int G, int B, int onTime, int offTime = 0, int times = 1)
{
    for (int i = 0; i < times; ++i)
    {
        ledRGB(R, G, B);
        vTaskDelay(pdMS_TO_TICKS(onTime));
        if (offTime > 0)
        {
            ledOff();
            vTaskDelay(pdMS_TO_TICKS(offTime));
        }
    }
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
            blink(STALE_R, STALE_G, STALE_B, 200);
        }
        else if (h <= HUMIDITY_MOIST)
        {
            Serial.printf("[NEO] COMFORT (%.2f%%) → Solid Green\n", h);
            blink(COMF_R, COMF_G, COMF_B, 1000);
        }
        else if (h <= HUMIDITY_WET)
        {
            Serial.printf("[NEO] MOIST (%.2f%%) → Blinking Yellow\n", h);
            blink(MOIST_R, MOIST_G, MOIST_B, 100, 100, 2);
            vTaskDelay(pdMS_TO_TICKS(600));
        }
        else
        {
            Serial.printf("[NEO] WET (%.2f%%) → Fast Red Blink\n", h);
            blink(WET_R, WET_G, WET_B, 100, 100, 3);
            vTaskDelay(pdMS_TO_TICKS(600));
        }
    }
}