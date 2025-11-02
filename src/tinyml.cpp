#include "tinyml.h"

namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];
}

int lightValue = 0;
int distance = 0;

Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init...");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model version %d != supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
    delay(1000);
    Serial.println("Reading light & sonar sensors...");
}

void tiny_ml_task(void *pvParameters)
{
    setupTinyML();

    while (1)
    {
        lightValue = analogRead(LIGHT_PIN);
        distance = ultrasonic.read();

        input->data.f[0] = lightValue;
        input->data.f[1] = distance;

        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
            vTaskDelay(5000);
            continue;
        }

        float prob0 = output->data.f[0];
        float prob1 = output->data.f[1];
        float prob2 = output->data.f[2];

        int predicted_label = 0;
        float max_conf = prob0;
        if (prob1 > max_conf)
        {
            predicted_label = 1;
            max_conf = prob1;
        }
        if (prob2 > max_conf)
        {
            predicted_label = 2;
            max_conf = prob2;
        }

        String label_str = "Safe.";
        switch (predicted_label)
        {
        case 1:
            label_str = "Caution!";
            break;
        case 2:
            label_str = "Alert!!";
            break;
        }

        Serial.printf("[ML] Light: %d | Distance: %d | Label: %s | Confidence: %.2f %% \n", lightValue, distance, label_str, max_conf*100);
        vTaskDelay(2000);
    }
}
