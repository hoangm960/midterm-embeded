#include "mainserver.h"

bool fan_state = false;
bool exit_sign_state = false;

WebServer server(80);

Adafruit_NeoPixel NeoPixel(4, EXIT_PIN, NEO_GRB + NEO_KHZ800);

String getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    return "text/plain";
}

String getFileETag(File &file)
{
    return "\"" + String(file.size()) + "\"";
}

void handleFileRead(String path)
{
    if (path.endsWith("/"))
        path += "index.html";

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        server.send(404, "text/plain", "File Not Found");
        return;
    }

    String etag = getFileETag(file);
    if (server.hasHeader("If-None-Match"))
    {
        String clientTag = server.header("If-None-Match");
        if (clientTag == etag)
        {
            server.send(304, "text/plain", "Not Modified");
            file.close();
            return;
        }
    }

    String contentType = getContentType(path);
    server.sendHeader("Cache-Control", "max-age=86400"); // 1 day
    server.sendHeader("ETag", etag);
    server.streamFile(file, contentType);
    file.close();
}

void handleRoot() { handleFileRead("/index.html"); }
void handleCSS() { handleFileRead("/style.css"); }
void handleJS() { handleFileRead("/script.js"); }

void handleToggle()
{
    String device = server.arg("device");

    if (device == "fan")
    {
        fan_state = !fan_state;
        digitalWrite(FAN_PIN, fan_state);
    }
    else if (device == "exit")
    {
        exit_sign_state = !exit_sign_state;
        uint32_t color = exit_sign_state ? NeoPixel.Color(0, 255, 0) : NeoPixel.Color(0, 0, 0);
        for (int i = 0; i < 3; i++)
        {
            NeoPixel.setPixelColor(i, color);
        }
        NeoPixel.show();
    }

    String jsonResponse = "{\"fan\":\"" + String(fan_state ? "ON" : "OFF") +
                          "\",\"exit\":\"" + String(exit_sign_state ? "ON" : "OFF") + "\"}";
    server.send(200, "application/json", jsonResponse);
}

void handleSensors()
{
    float t, h;

    if (xSemaphoreTake(xGlobalDataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        t = glob_temperature;
        h = glob_humidity;
        xSemaphoreGive(xGlobalDataMutex);
    }
    else
    {
        t = -99.9;
        h = -99.9;
        Serial.println("[Server] Failed to get global data mutex in handleSensors");
    }

    String json = "{\"temp\":" + String(t) + ",\"hum\":" + String(h) + "}";
    server.send(200, "application/json", json);
}

void setupServer()
{
    server.onNotFound([]()
                      { handleFileRead(server.uri()); });
    server.on("/toggle", HTTP_GET, handleToggle);
    server.on("/sensors", HTTP_GET, handleSensors);
    server.begin();
}

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), password.c_str());
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}

void main_server_task(void *pvParameters)
{
    if (!LittleFS.begin(true))
    {
        Serial.println("An error occurred while mounting LittleFS");
        return;
    }

    pinMode(FAN_PIN, OUTPUT);
    NeoPixel.begin();
    NeoPixel.show();

    startAP();
    setupServer();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        server.handleClient();
    }
}