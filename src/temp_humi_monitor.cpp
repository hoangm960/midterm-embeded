#include "temp_humi_monitor.h"

// Thresholds
#define TEMP_LOW 25  // lower threshold, can be changed
#define TEMP_HIGH 28 // higher threshold

// Globals
DHT20 dht20;
LiquidCrystal_I2C lcd(33, 16, 2);

SemaphoreHandle_t alarmSemaphore;
float lastTemperature = 0.0;
float lastHumidity = 0.0;
volatile int alarmState = 0; // 0: normal, 1: low, 2: high

void SensorTask(void *pvParameters)
{
    for (;;)
    {
        dht20.read();

        Serial.print("Temperature: ");
        Serial.print(lastTemperature);
        Serial.print(" C, Humidity: ");
        Serial.print(lastHumidity);
        Serial.println(" %");

        lastTemperature = dht20.getTemperature();
        lastHumidity = dht20.getHumidity();

        // Check thresholds
        if (lastTemperature < TEMP_LOW)
        {
            alarmState = 1;
            xSemaphoreGive(alarmSemaphore); // Signal alarm task
        }
        else if (lastTemperature > TEMP_HIGH)
        {
            alarmState = 2;
            xSemaphoreGive(alarmSemaphore);
        }
        else
        {
            alarmState = 0;
            xSemaphoreGive(alarmSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void DisplayTask(void *pvParameters)
{
    for (;;)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(lastTemperature);
        lcd.setCursor(0, 1);
        lcd.print("Hum: ");
        lcd.print(lastHumidity);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void AlarmTask(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(alarmSemaphore, portMAX_DELAY) == pdTRUE)
        {
            if (alarmState == 1)
            {
                Serial.print("Low Temp Alarm! ");
            }
            else if (alarmState == 2)
            {
                Serial.print("High Temp Alarm! ");
            }
            else
            {
                Serial.print("Temperature Normal. ");
            }
        }
    }
}

void temp_humi_monitor(void *pvParameters)
{
    Serial.begin(115200);
    Wire.begin(11, 12);
    dht20.begin();
    lcd.begin();

    alarmSemaphore = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(SensorTask, "setLED", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(SensorTask, "SensorTask", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(DisplayTask, "DisplayTask", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(AlarmTask, "AlarmTask", 2048, NULL, 1, NULL, 1);
}
