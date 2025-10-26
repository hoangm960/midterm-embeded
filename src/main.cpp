#include <Arduino.h>
#include <Wire.h>
#include <DHT20.h>

// ---------------- Pin / HW ----------------
#define LED_PIN   48        // On-board (GPIO_NUM_48)
#define SDA_PIN   11        // Yolo UNO I2C
#define SCL_PIN   12

// --------- Temperature thresholds ---------
#define TEMP_COOL_MAX 37.0  // < 37  → slow single blink
#define TEMP_WARM_MAX 40.0  // 40 → double blink
                            // > 40  → triple blink

// -------------- Shared state --------------
float lastTemperature = NAN;
float lastHumidity    = NAN;

// -------------- Tasks ---------------------
void TaskReadSensor(void *pvParameters) {
  // Make the sensor instance LOCAL so it doesn't clash with any other file.
  static DHT20 dht20;
  static bool inited = false;

  if (!inited) {
    // dht20 uses the global Wire — make sure Wire.begin() ran in setup()
    dht20.begin();
    inited = true;
  }

  for (;;) {
    if (dht20.read() == 0) {
      lastTemperature = dht20.getTemperature();
      lastHumidity    = dht20.getHumidity();
      Serial.printf("[SENSOR] Temperature: %.2f °C, Humidity: %.2f %%\n",
                    lastTemperature, lastHumidity);
    } else {
      lastTemperature = NAN;   // mark error
    }
    vTaskDelay(pdMS_TO_TICKS(3000));   // every 3 s
  }
}

void TaskLEDControl(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("=== ESP32-S3 LED Blink with Temperature Conditions ===");

  for (;;) {
    float t = lastTemperature;

    if (isnan(t)) {
      // Sensor error → long ON
      Serial.println("[LED] COOL (0.00°C) → slow blink"); // first line you wanted kept
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(300));
    }
    else if (t < TEMP_COOL_MAX) {
      // COOL → slow single blink
      Serial.println("[LED] COOL (0.00°C) → slow blink"); // minimal text
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(800));
    }
    else if (t < TEMP_WARM_MAX) {
      // WARM → double blink
      // (keep prints minimal so you only see a couple lines)
      for (int i = 0; i < 2; i++) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(150));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(150));
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    else {
      // HOT → triple blink
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(120));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(120));
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

// ----------------- Setup / Loop -----------------
void setup() {
  Serial.begin(115200);
  delay(500);

  // I2C for DHT20
  Wire.begin(SDA_PIN, SCL_PIN);

  // Create tasks
  // Increase stacks to avoid stack-canary resets
  xTaskCreate(TaskLEDControl, "LEDControl", 6144, NULL, 2, NULL);
  xTaskCreate(TaskReadSensor, "ReadSensor", 4096, NULL, 2, NULL);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
