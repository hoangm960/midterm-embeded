// temp_humi_monitor_fixed.cpp
#include "temp_humi_monitor.h"
#include <Arduino.h>
#include <Wire.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>

#define TEMP_LOW 25.0f
#define TEMP_HIGH 28.0f
#define SENSOR_DELAY_MS 5000

typedef struct
{
    float temperature;
    float humidity;
} SensorData_t;

DHT20 dht20;
LiquidCrystal_I2C lcd(33, 16, 2);

static SemaphoreHandle_t i2cMutex = NULL;
static QueueHandle_t sensorQueue = NULL;
static SemaphoreHandle_t alarmSemaphore = NULL;

volatile int alarmState = 0; // 0: normal, 1: low, 2: high

void SensorTask(void *pvParameters)
{
    static int prevAlarmState = -1;

    for (;;)
    {
        float t = NAN, h = NAN;

        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(2000)) == pdTRUE)
        {
            dht20.read();
            t = dht20.getTemperature();
            h = dht20.getHumidity();
            xSemaphoreGive(i2cMutex);
        }
        else
        {
            Serial.println("I2C mutex timeout in SensorTask");
        }

        if (!isnan(t) && !isnan(h))
        {
            SensorData_t data = {t, h};
            if (xQueueOverwrite(sensorQueue, &data) != pdPASS)
            {
                Serial.println("Failed to write sensorQueue");
            }

            int newState = 0;
            if (t < TEMP_LOW)
                newState = 1;
            else if (t > TEMP_HIGH)
                newState = 2;

            if (newState != prevAlarmState)
            {
                alarmState = newState;
                xSemaphoreGive(alarmSemaphore);
                prevAlarmState = newState;
            }

            Serial.printf("Temperature: %.2f C, Humidity: %.2f %%\n", t, h);
        }
        else
        {
            Serial.println("Sensor read failed (NaN)");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_DELAY_MS));
    }
}

void DisplayTask(void *pvParameters)
{
    SensorData_t data;
    SensorData_t prevData = {-1000.0f, -1000.0f};

    for (;;)
    {
        if (xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdTRUE)
        {
            if (fabs(data.temperature - prevData.temperature) > 0.1f ||
                fabs(data.humidity - prevData.humidity) > 0.1f)
            {

                if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(2000)) == pdTRUE)
                {
                    lcd.setCursor(0, 0);
                    
                    switch (alarmState)
                    {
                    case 1:
                        lcd.print("Warning");
                        break;
                    case 2:
                        lcd.print("CRITICAL");
                        break;
                    default:
                        lcd.print("Normal");
                        break;
                    }

                    lcd.setCursor(0, 1);
                    lcd.print(data.temperature, 1);
                    lcd.print("C   ");
                    lcd.print(data.humidity, 1);
                    lcd.print("%   ");
                    xSemaphoreGive(i2cMutex);
                }
                else
                {
                    Serial.println("I2C mutex timeout in DisplayTask");
                }

                prevData = data;
            }
        }
    }
}

void AlarmTask(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(alarmSemaphore, portMAX_DELAY) == pdTRUE)
        {
            switch (alarmState)
            {
            case 1:
                Serial.println("Low Temperature Alarm!");
                break;
            case 2:
                Serial.println("High Temperature Alarm!");
                break;
            default:
                Serial.println("Temperature Normal.");
                break;
            }
        }
    }
}

void temp_humi_monitor(void *pvParameters)
{
    Serial.begin(115200);
    delay(50);
    Wire.begin(11, 12);

    if (!dht20.begin())
    {
        Serial.println("DHT20 begin failed");
    }
    lcd.begin();
    lcd.backlight();

    i2cMutex = xSemaphoreCreateMutex();
    alarmSemaphore = xSemaphoreCreateBinary();
    sensorQueue = xQueueCreate(1, sizeof(SensorData_t));

    if (!i2cMutex || !alarmSemaphore || !sensorQueue)
    {
        Serial.println("Failed to create RTOS resources! Halting.");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    BaseType_t r;

    r = xTaskCreatePinnedToCore(SensorTask, "SensorTask", 8192, NULL, 2, NULL, 1);
    if (r != pdPASS)
    {
        Serial.println("Failed to create SensorTask");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    r = xTaskCreatePinnedToCore(DisplayTask, "DisplayTask", 6144, NULL, 1, NULL, 1);
    if (r != pdPASS)
    {
        Serial.println("Failed to create DisplayTask");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    r = xTaskCreatePinnedToCore(AlarmTask, "AlarmTask", 4096, NULL, 1, NULL, 1);
    if (r != pdPASS)
    {
        Serial.println("Failed to create AlarmTask");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    Serial.println("Temperature/Humidity monitor started.");

    vTaskDelete(NULL);
}
