#include <Arduino.h>
#include <Wire.h>
#include <DHT20.h>
#include <Adafruit_NeoPixel.h>

// ---------------- Pin / HW ----------------
#define NEOPIXEL_PIN 45     // NeoPixel pin
#define SDA_PIN      11     // I2C SDA
#define SCL_PIN      12     // I2C SCL

// neopixel set up
#define NUM_PIXELS   1
#define BRIGHTNESS   120
Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

static inline void ledRGB(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}
static inline void ledOff() { ledRGB(0, 0, 0); }

// semaphore
static SemaphoreHandle_t xNewSampleSem = NULL;   
static SemaphoreHandle_t xDataMutex    = NULL;  

static float lastHumidity = NAN;

// safe access
static void setHumidity(float h) {
  if (!xDataMutex) return;
  xSemaphoreTake(xDataMutex, portMAX_DELAY);
  lastHumidity = h;
  xSemaphoreGive(xDataMutex);
}

static float getHumidity() {
  float h = NAN;
  if (xDataMutex) {
    xSemaphoreTake(xDataMutex, portMAX_DELAY);
    h = lastHumidity;
    xSemaphoreGive(xDataMutex);
  }
  return h;
}

// humid and color
constexpr float HUM_DRY_MAX     = 40.0f;   // ≤ 40% → Blue
constexpr float HUM_COMFORT_MAX = 60.0f;   // 40–60% → Green
constexpr float HUM_MOIST_MAX   = 80.0f;   // 60–80% → Yellow
// > 80% → Red

static const uint8_t
  DRY_R = 0,   DRY_G = 0,   DRY_B = 200,     // Blue
  COMF_R = 0,  COMF_G = 200, COMF_B = 0,      // Green
  MOIST_R = 255, MOIST_G = 200, MOIST_B = 0, // Yellow
  WET_R = 255, WET_G = 0,   WET_B = 0,       // Red
  STALE_R = 150, STALE_G = 0, STALE_B = 150; // Purple

// tasks
void TaskReadSensor(void *pvParameters) {
  static DHT20 dht20;
  static bool inited = false;

  if (!inited) {
    dht20.begin();  
    inited = true;
  }

  for (;;) {
    if (dht20.read() == 0) {
      float h = dht20.getHumidity();

      setHumidity(h);                     // <-- protected write
      if (xNewSampleSem) xSemaphoreGive(xNewSampleSem);  // signal LED task

      Serial.printf("[SENSOR] Humidity: %.2f %%\n", h);
    } else {
      setHumidity(NAN);                   // mark error
      if (xNewSampleSem) xSemaphoreGive(xNewSampleSem);
      Serial.println("[SENSOR] Read FAILED");
    }

    vTaskDelay(pdMS_TO_TICKS(3000));  // every 3 seconds
  }
}

void TaskLEDControl(void *pvParameters) {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  Serial.println("ESP32-S3 NeoPixel Humidity Indicator ===");

  // Startup animation
  ledRGB(200, 0, 0);   vTaskDelay(150);
  ledRGB(0, 200, 0);   vTaskDelay(150);
  ledRGB(0, 0, 200);   vTaskDelay(150);
  ledOff();

  bool blink = false;

  for (;;) {
    // new humidnity sample
    if (xSemaphoreTake(xNewSampleSem, pdMS_TO_TICKS(800)) != pdTRUE) {
      // Timeout → stale
      Serial.println("[LED] STALE → Purple pulse");
      ledRGB(STALE_R, STALE_G, STALE_B); vTaskDelay(200);
      ledOff(); vTaskDelay(200);
      ledRGB(STALE_R, STALE_G, STALE_B); vTaskDelay(200);
      ledOff(); vTaskDelay(800);
      continue;
    }

    float h = getHumidity();  // <-- protected read

    if (isnan(h)) {
      // Sensor error
      Serial.println("[LED] ERROR → Purple pulse");
      ledRGB(STALE_R, STALE_G, STALE_B); vTaskDelay(200);
      ledOff(); vTaskDelay(200);
      ledRGB(STALE_R, STALE_G, STALE_B); vTaskDelay(200);
      ledOff(); vTaskDelay(800);
    }
    else if (h <= HUM_DRY_MAX) {
      Serial.printf("[LED] DRY (%.2f%%) → Solid Blue\n", h);
      ledRGB(DRY_R, DRY_G, DRY_B);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else if (h <= HUM_COMFORT_MAX) {
      Serial.printf("[LED] COMFORT (%.2f%%) → Solid Green\n", h);
      ledRGB(COMF_R, COMF_G, COMF_B);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else if (h <= HUM_MOIST_MAX) {
      Serial.printf("[LED] MOIST (%.2f%%) → Blinking Yellow\n", h);
      blink = !blink;
      if (blink) ledRGB(MOIST_R, MOIST_G, MOIST_B);
      else ledOff();
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    else {
      Serial.printf("[LED] WET (%.2f%%) → Fast Red Blink\n", h);
      for (int i = 0; i < 3; ++i) {
        ledRGB(WET_R, WET_G, WET_B); vTaskDelay(100);
        ledOff(); vTaskDelay(100);
      }
      vTaskDelay(pdMS_TO_TICKS(600));
    }
  }
}

// ----------------- Setup / Loop -----------------
void setup() {
  Serial.begin(115200);
  delay(500);

  // I2C for DHT20
  Wire.begin(SDA_PIN, SCL_PIN);

  // Create synchronization objects
  xNewSampleSem = xSemaphoreCreateBinary();
  xDataMutex    = xSemaphoreCreateMutex();

  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  // Create tasks
  xTaskCreate(TaskLEDControl, "LEDControl", 6144, NULL, 2, NULL);
  xTaskCreate(TaskReadSensor, "ReadSensor", 4096, NULL, 2, NULL);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}