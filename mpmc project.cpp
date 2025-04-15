#include <WiFi.h>
#include <MQ135.h>
#include <DHT.h>
#include <ThingSpeak.h>

// Pin Definitions
#define MQ135_PIN 34
#define DHTPIN 18
#define DHTTYPE DHT11

// WiFi Credentials
const char* ssid = "Jai"; // Replace with your WiFi SSID
const char* password = "Jaijishnu11"; // Replace with your WiFi Password

// ThingSpeak Credentials
unsigned long myChannelNumber = 2915306; // Replace with your ThingSpeak channel number
const char* myWriteAPIKey = "EZ8I894Y9XE7CNNI";    // Replace with your ThingSpeak write API key

// Initialize Sensors
MQ135 mq135(MQ135_PIN);
DHT dht(DHTPIN, DHTTYPE);

// Create a WiFi client for ThingSpeak
WiFiClient client;

void setup() {
  Serial.begin(115200);

  // Initialize Sensors
  dht.begin();

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
  } else {
    Serial.println("\nWiFi Connection Failed. Please check credentials!");
    return; // Exit setup if WiFi fails
  }

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // Read Sensor Data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float gasPPM = mq135.getPPM();

  // Validate Sensor Readings
  if (isnan(temperature) || isnan(humidity) || gasPPM <= 0) {
    Serial.println("⚠️ Invalid sensor readings detected! Skipping transmission.");
    delay(10000);
    return; // Skip sending data if readings are invalid
  }

  // Print Sensor Data to Serial Monitor
  Serial.print("🌡️ Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("💧 Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("🛢️ Gas PPM: "); Serial.print(gasPPM); Serial.println(" PPM");

  // Send Data to ThingSpeak
  ThingSpeak.setField(1, temperature); // Update field 1 with temperature
  ThingSpeak.setField(2, humidity);    // Update field 2 with humidity
  ThingSpeak.setField(3, gasPPM);      // Update field 3 with gas PPM

  int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (responseCode == 200) { 
    Serial.println("✅ Data successfully sent to ThingSpeak!");
  } else { 
    Serial.print("❌ Error sending data to ThingSpeak. HTTP Response Code: ");
    Serial.println(responseCode);
  }

  delay(10000); // Wait 10 seconds before the next reading
}