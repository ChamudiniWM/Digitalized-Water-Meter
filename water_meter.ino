#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <FS.h>

// WiFi credentials
const char* ssid = "realme C21Y";
const char* password = "djdjdjdj";

// ThingSpeak settings
const char* thingSpeakApiKey = "TB0CCCPTVQL044WJ";
const unsigned long thingSpeakChannel = 2292075;

// Define the GPIO pin to which the flow sensor is connected
const int flowSensorPin = D2;

// Flow sensor calibration factor (pulses to liters)
const float PULSES_TO_LITERS = 0.00222;

unsigned int flowPulseCount = 0;
float totalWaterUsage = 0.0; // Total water usage in liters
float flowRate = 0.0;       // Flow rate in L/min
unsigned long prevMillis = 0;

bool wifiConnected = false;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  pinMode(flowSensorPin, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  ThingSpeak.begin(client);

  // Initialize SPIFFS (you may need to format it the first time)
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed!");
  } else {
    Serial.println("SPIFFS initialized successfully.");
  }

  // Read totalWaterUsage from SPIFFS
  File file = SPIFFS.open("/water_usage.txt", "r");
  if (file) {
    totalWaterUsage = file.parseFloat();
    file.close();
  }

  wifiConnected = true;
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - prevMillis >= 1000) { // Update every 1 second
    // Calculate flow rate in L/min
    flowRate = (flowPulseCount / 7.5 / 1000); // Adjust the calibration factor as needed

    // Display flow rate
    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.println(" L/min");

    // Calculate total water usage in liters
    totalWaterUsage += (flowRate * (currentMillis - prevMillis) / 60000.0); // Calculate water usage in liters

    // Display total water usage
    Serial.print("Total Water Usage: ");
    Serial.print(totalWaterUsage);
    Serial.println(" liters");

    if (wifiConnected) {
      // Send data to ThingSpeak
      ThingSpeak.setField(2, flowRate);
      ThingSpeak.setField(1, totalWaterUsage);
      int httpCode = ThingSpeak.writeFields(thingSpeakChannel, thingSpeakApiKey);
      if (httpCode == 200) {
        Serial.println("Data sent to ThingSpeak successfully");
      } else {
        Serial.println("Error sending data to ThingSpeak");
      }
    }

    // Save totalWaterUsage to SPIFFS
    File file = SPIFFS.open("/water_usage.txt", "w");
    if (file) {
      file.print(totalWaterUsage);
      file.close();
    }

    flowPulseCount = 0;
    prevMillis = currentMillis;
  }

  if (digitalRead(flowSensorPin) == LOW) {
    flowPulseCount++;
  }
}
