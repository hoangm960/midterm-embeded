#include "temp_humi_monitor.h"

typedef struct
{
    float temperature;
    float humidity;
    int alarmState;
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
            if (dht20.read() == 0)
            {
                t = dht20.getTemperature();
                h = dht20.getHumidity();
            }
            xSemaphoreGive(i2cMutex);
        }
        else
        {
            Serial.println("I2C mutex timeout in SensorTask");
        }

        if (!isnan(t) && !isnan(h))
        {
            int newState = 0;
            if (t < TEMP_LOW)
                newState = 1;
            else if (t > TEMP_HIGH)
                newState = 2;

            SensorData_t data = {t, h, newState};
            if (xQueueOverwrite(sensorQueue, &data) != pdPASS)
            {
                Serial.println("Failed to write sensorQueue");
            }

            if (xSemaphoreTake(xGlobalDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                glob_temperature = t;
                glob_humidity = h;
                xSemaphoreGive(xGlobalDataMutex);
            }
            else
            {
                Serial.println("Global data mutex timeout in SensorTask");
            }
            
            xSemaphoreGive(xNewSampleSem);

            if (newState != prevAlarmState)
            {
                alarmState = newState;
                xSemaphoreGive(alarmSemaphore);
                prevAlarmState = newState;
            }

            Serial.printf("[SensorTask] Temp: %.2f C, Humi: %.2f %% \n", t, h);
        }
        else
        {
            Serial.println("Sensor read failed (NaN)");
            xSemaphoreGive(xNewSampleSem);
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_DELAY_MS));
    }
}

void DisplayTask(void *pvParameters)
{
    SensorData_t data;
    SensorData_t prevData = {-1000.0f, -1000.0f, 0};

    for (;;)
    {
        if (xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdTRUE)
        {
            if (fabs(data.temperature - prevData.temperature) > 0.1f ||
                fabs(data.humidity - prevData.humidity) > 0.1f ||
                data.alarmState != prevData.alarmState)
            {

                if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(2000)) == pdTRUE)
                {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    
                    switch (data.alarmState)
                    {
                    case 1:
                        lcd.print("Status: Warning");
                        break;
                    case 2:
                        lcd.print("Status: CRITICAL");
                        break;
                    default:
                        lcd.print("Status: Normal");
                        break;
                    }

                    lcd.setCursor(0, 1);
                    lcd.print("T:");
                    lcd.print(data.temperature, 1);
                    lcd.print("C H:");
                    lcd.print(data.humidity, 1);
                    lcd.print("%");
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
                Serial.println("ALARM: Temperature LOW!");
                break;
            case 2:
                Serial.println("ALARM: Temperature HIGH!");
                break;
            default:
                Serial.println("ALARM: Temperature Normal.");
                break;
            }
        }
    }
}

void temp_humi_monitor(void *pvParameters)
{
    Wire.begin(SDA_PIN, SCL_PIN);

    if (!dht20.begin())
    {
        Serial.println("DHT20 begin failed");
    }
    lcd.begin();
    lcd.backlight();
    lcd.clear();
    lcd.print("System Booting...");

    i2cMutex = xSemaphoreCreateMutex();
    alarmSemaphore = xSemaphoreCreateBinary();
    sensorQueue = xQueueCreate(1, sizeof(SensorData_t));

    if (!i2cMutex || !alarmSemaphore || !sensorQueue || !xGlobalDataMutex || !xNewSampleSem)
    {
        Serial.println("Failed to create RTOS resources! Halting.");
        lcd.clear();
        lcd.print("RTOS Boot Fail!");
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    BaseType_t r;

    r = xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, NULL, 2, NULL, 1);
    if (r != pdPASS)
    {
        Serial.println("Failed to create SensorTask");
    }

    r = xTaskCreatePinnedToCore(DisplayTask, "DisplayTask", 4096, NULL, 1, NULL, 1);
    if (r != pdPASS)
    {
        Serial.println("Failed to create DisplayTask");
    }

    r = xTaskCreatePinnedToCore(AlarmTask, "AlarmTask", 2048, NULL, 1, NULL, 1);
    if (r != pdPASS)
    {
        Serial.println("Failed to create AlarmTask");
    }

    Serial.println("Temperature/Humidity monitor module started.");
    vTaskDelete(NULL);
}