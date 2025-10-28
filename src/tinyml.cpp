#include "tinyml.h"
// #include <Ultrasonic.h>

// Globals for TensorFlow Lite Micro
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on model size
    uint8_t tensor_arena[kTensorArenaSize];
} // namespace

// Light sensor setup
const int lightPin = 3;
int lightValue = 0;

// Ultrasonic sensor setup
int triggerPin = 1;
int echoPin = 2;
int distance = 0;
Ultrasonic ultrasonic(triggerPin, echoPin);

void setupTinyML()
{
    Serial.begin(115200);
    Serial.println("TensorFlow Lite Init...");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // Load model (replace with your model array variable name)
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
        // --- Read sensors ---
        lightValue = analogRead(lightPin); // 0–4095
        distance = ultrasonic.read();      // in cm

        // --- Normalize if needed ---
        input->data.f[0] = lightValue;
        input->data.f[1] = distance;

        // --- Run inference ---
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
            vTaskDelay(5000);
            continue;
        }

        // --- Get 3 output probabilities ---
        float prob0 = output->data.f[0];
        float prob1 = output->data.f[1];
        float prob2 = output->data.f[2];

        // --- Determine predicted label ---
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

        // --- Print clearly ---
        Serial.print("Light: ");
        Serial.print(lightValue);
        Serial.print(" | Distance: ");
        Serial.print(distance);
        Serial.print(" | Label: ");
        Serial.print(predicted_label);
        Serial.print(" | Confidence: ");
        Serial.print(max_conf * 100, 2);
        Serial.println("%");

        Serial.flush();   // Ensure all data is sent out cleanly
        vTaskDelay(2000); // 5-second delay
    }
}
