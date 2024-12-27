#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// WiFi credentials
const char* ssid = "nesyaaa";
const char* password = "aaaaaaab";

// Pin definitions
const int LED_PIN = 4;  // GPIO4
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// Global variables
bool isBlinking = false;
unsigned long previousMillis = 0;
const long blinkInterval = 1000;  // Blink interval in milliseconds

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Function declarations
void handleLED(bool state);
void setupPWM();
void handleBlink();

void setup() {
    Serial.begin(115200);

    // Initialize SPIFFS
    if(!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    // Configure LED pin
    pinMode(LED_PIN, OUTPUT);
    setupPWM();

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });

    // Route to handle LED control
    server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
        isBlinking = false;
        handleLED(true);
        request->send(200, "text/plain", "LED turned ON");
    });

    server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request){
        isBlinking = false;
        handleLED(false);
        request->send(200, "text/plain", "LED turned OFF");
    });

    server.on("/led/blink", HTTP_GET, [](AsyncWebServerRequest *request){
        isBlinking = true;
        request->send(200, "text/plain", "LED set to BLINK");
    });

    // Route to handle PWM brightness control
    server.on("/pwm/^[0-9]+$", HTTP_GET, [](AsyncWebServerRequest *request){
        String brightnessStr = request->url().substring(5);
        int brightness = brightnessStr.toInt();
        if(brightness >= 0 && brightness <= 255) {
            ledcWrite(PWM_CHANNEL, brightness);
            request->send(200, "text/plain", "Brightness set to " + String(brightness));
        } else {
            request->send(400, "text/plain", "Invalid brightness value");
        }
    });

    // Route to handle device reboot
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Rebooting...");
        delay(1000);
        ESP.restart();
    });

    // Start server
    server.begin();
}

void loop() {
    if (isBlinking) {
        handleBlink();
    }
}

void setupPWM() {
    // Configure PWM channel
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 128);  // Set initial brightness to 50%
}

void handleLED(bool state) {
    if (state) {
        ledcWrite(PWM_CHANNEL, 255);  // Full brightness
    } else {
        ledcWrite(PWM_CHANNEL, 0);    // Off
    }
}

void handleBlink() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
        previousMillis = currentMillis;
        static bool ledState = false;
        ledState = !ledState;
        handleLED(ledState);
    }
}