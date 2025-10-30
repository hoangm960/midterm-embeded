#include "temp_humi_monitor.h"

DHT20 dht20;
// LiquidCrystal_I2C lcd(33, 16, 2); // Uncomment if you have an LCD

void temp_humi_monitor(void *pvParameters)
{
  // This task is now the ONLY one talking to the sensor
  Wire.begin(SDA_PIN, SCL_PIN); // Initialize I2C here
  dht20.begin();
  // lcd.begin(); // Initialize LCD here if you have one

  while (1)
  {
    float temperature, humidity;

    if (dht20.read() == 0)
    {
      temperature = dht20.getTemperature();
      humidity = dht20.getHumidity();
    }
    else
    {
      Serial.println("[SENSOR] Failed to read from DHT sensor!");
      temperature = NAN;
      humidity = NAN;
    }

    // FIX: Thread-safe update of global variables
    if (xSemaphoreTake(xGlobalDataMutex, portMAX_DELAY) == pdTRUE)
    {
      glob_temperature = temperature;
      glob_humidity = humidity;
      xSemaphoreGive(xGlobalDataMutex);
    }

    // FIX: Signal the LED task that a new sample is available
    xSemaphoreGive(xNewSampleSem);

    // Print the results
    Serial.print("[SENSOR] H: ");
    Serial.print(humidity);
    Serial.print("%  T: ");
    Serial.print(temperature);
    Serial.println("°C");

    /*
    // Example LCD update
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(temperature) + "C");
    lcd.setCursor(0, 1);
    lcd.print("Humi: " + String(humidity) + "%");
    */

    vTaskDelay(pdMS_TO_TICKS(3000)); // Sample every 3 seconds
  }
}