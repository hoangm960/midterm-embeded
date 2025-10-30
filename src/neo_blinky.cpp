#include "neo_blinky.h"

// Initialize the strip from the header definitions
Adafruit_NeoPixel strip(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

static inline void ledRGB(uint8_t r, uint8_t g, uint8_t b)
{
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}
static inline void ledOff() { ledRGB(0, 0, 0); }

// Humidity and color constants
constexpr float HUM_DRY_MAX = 40.0f;     // ≤ 40% → Blue
constexpr float HUM_COMFORT_MAX = 60.0f; // 40–60% → Green
constexpr float HUM_MOIST_MAX = 80.0f;   // 60–80% → Yellow
// > 80% → Red

static const uint8_t
    DRY_R = 0,
    DRY_G = 0, DRY_B = 200,                    // Blue
    COMF_R = 0, COMF_G = 200, COMF_B = 0,      // Green
    MOIST_R = 255, MOIST_G = 200, MOIST_B = 0, // Yellow
    WET_R = 255, WET_G = 0, WET_B = 0,         // Red
    STALE_R = 150, STALE_G = 0, STALE_B = 150; // Purple

// FIX: ---- DELETED local semaphores, mutex, and sensor task ----

// FIX: New helper function to safely read the GLOBAL humidity
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

// tasks
void TaskNeoPixelControl(void *pvParameters) // Renamed from TaskLEDControl
{
    strip.begin();
    strip.setBrightness(BRIGHTNESS);
    strip.show();

    Serial.println("=== NeoPixel Humidity Indicator Started ===");

    // Startup animation
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
        // FIX: Wait on the GLOBAL semaphore from the sensor task
        if (xSemaphoreTake(xNewSampleSem, pdMS_TO_TICKS(8000)) != pdTRUE)
        {
            // Timeout → stale
            Serial.println("[NEO] STALE → Purple pulse");
            ledRGB(STALE_R, STALE_G, STALE_B);
            vTaskDelay(200);
            ledOff();
            // ... (rest of stale animation)
            vTaskDelay(800);
            continue;
        }

        // FIX: Read humidity from the new safe-access function
        float h = getSafeHumidity();

        if (isnan(h))
        {
            // Sensor error
            Serial.println("[NEO] ERROR → Purple pulse");
            ledRGB(STALE_R, STALE_G, STALE_B);
            vTaskDelay(200);
            ledOff();
            // ... (rest of error animation)
        }
        else if (h <= HUM_DRY_MAX)
        {
            Serial.printf("[NEO] DRY (%.2f%%) → Solid Blue\n", h);
            ledRGB(DRY_R, DRY_G, DRY_B);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else if (h <= HUM_COMFORT_MAX)
        {
            Serial.printf("[NEO] COMFORT (%.2f%%) → Solid Green\n", h);
            ledRGB(COMF_R, COMF_G, COMF_B);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else if (h <= HUM_MOIST_MAX)
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

// This is the main "setup" task called from main.cpp
void neo_blinky(void *pvParameters)
{
    // FIX: Remove Serial.begin() (already in main)
    // FIX: Remove Wire.begin() (handled by temp_humi_monitor)
    // FIX: Remove mutex/semaphore creation (they are global)

    // Initialize NeoPixel
    strip.begin();
    strip.setBrightness(BRIGHTNESS);
    strip.show();

    // Create tasks
    xTaskCreate(TaskNeoPixelControl, "NeoPixelControl", 4096, NULL, 2, NULL);

    // FIX: Remove TaskReadSensor creation

    // FIX: This setup task is done, so it deletes itself to free resources
    vTaskDelete(NULL);
}