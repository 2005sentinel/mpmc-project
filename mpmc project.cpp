#include <WiFi.h>
#include <MQ135.h>
#include <DHT.h>
#include <ThingSpeak.h>

// Pin Definitions
#define MQ135_PIN 34
#define DHTPIN 18
#define DHTTYPE DHT11
#define SOLAR_VOLTAGE_PIN 35    // Analog pin for solar voltage measurement
#define BATTERY_VOLTAGE_PIN 32  // Analog pin for battery voltage measurement
#define CURRENT_SENSOR_PIN 33   // Analog pin for current measurement

// WiFi Credentials
const char* ssid = "Jai";
const char* password = "Jaijishnu11";

// ThingSpeak Credentials
unsigned long myChannelNumber = 2915306;
const char* myWriteAPIKey = "EZ8I894Y9XE7CNNI";

// Initialize Sensors
MQ135 mq135(MQ135_PIN);
DHT dht(DHTPIN, DHTTYPE);

// Power Monitoring Variables
float solarVoltage = 0;
float batteryVoltage = 0;
float currentDraw = 0;
float powerGenerated = 0;
float powerConsumed = 0;
unsigned long lastMeasurementTime = 0;
float totalEnergyGenerated = 0;
float totalEnergyConsumed = 0;

// Calibration values (adjust based on your hardware)
#define VOLTAGE_DIVIDER_RATIO 0.5  // For voltage measurement
#define CURRENT_SENSOR_SENSITIVITY 0.1  // V/A for current sensor
#define SHUNT_RESISTOR 0.1         // Ohms

WiFiClient client;

void setup() {
  Serial.begin(115200);

  // Initialize Sensors
  dht.begin();
  pinMode(SOLAR_VOLTAGE_PIN, INPUT);
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);
  pinMode(CURRENT_SENSOR_PIN, INPUT);

  // Connect to WiFi
  connectToWiFi();

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void connectToWiFi() {
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
  }
}

void measurePower() {
  // Read solar panel voltage (through voltage divider)
  int solarRaw = analogRead(SOLAR_VOLTAGE_PIN);
  solarVoltage = (solarRaw / 4095.0) * 3.3 / VOLTAGE_DIVIDER_RATIO;

  // Read battery voltage (through voltage divider)
  int batteryRaw = analogRead(BATTERY_VOLTAGE_PIN);
  batteryVoltage = (batteryRaw / 4095.0) * 3.3 / VOLTAGE_DIVIDER_RATIO;

  // Read current draw (using current sensor)
  int currentRaw = analogRead(CURRENT_SENSOR_PIN);
  float voltageAcrossShunt = (currentRaw / 4095.0) * 3.3;
  currentDraw = (voltageAcrossShunt - 1.65) / CURRENT_SENSOR_SENSITIVITY; // 1.65V is zero point for bidirectional sensors

  // Calculate power
  powerGenerated = solarVoltage * currentDraw;  // Simplified - actual calculation depends on your setup
  powerConsumed = batteryVoltage * currentDraw;

  // Calculate energy (integrate power over time)
  unsigned long currentTime = millis();
  float deltaTimeHours = (currentTime - lastMeasurementTime) / 3600000.0; // Convert ms to hours
  totalEnergyGenerated += powerGenerated * deltaTimeHours; // Wh
  totalEnergyConsumed += powerConsumed * deltaTimeHours;   // Wh
  lastMeasurementTime = currentTime;
}

void loop() {
  // Measure power parameters first
  measurePower();

  // Read Sensor Data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float gasPPM = mq135.getPPM();

  // Validate Sensor Readings
  if (isnan(temperature) || isnan(humidity) || gasPPM <= 0) {
    Serial.println("⚠️ Invalid sensor readings detected! Skipping transmission.");
    delay(10000);
    return;
  }

  // Print Sensor Data to Serial Monitor
  Serial.println("\n------ Sensor Readings ------");
  Serial.print("🌡️ Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("💧 Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("🛢️ Gas PPM: "); Serial.print(gasPPM); Serial.println(" PPM");

  // Print Power Data to Serial Monitor
  Serial.println("\n------ Power Statistics ------");
  Serial.print("☀️ Solar Voltage: "); Serial.print(solarVoltage); Serial.println(" V");
  Serial.print("🔋 Battery Voltage: "); Serial.print(batteryVoltage); Serial.println(" V");
  Serial.print("⚡ Current Draw: "); Serial.print(currentDraw * 1000); Serial.println(" mA");
  Serial.print("🔌 Power Generated: "); Serial.print(powerGenerated); Serial.println(" W");
  Serial.print("🔋 Power Consumed: "); Serial.print(powerConsumed); Serial.println(" W");
  Serial.print("📈 Total Energy Generated: "); Serial.print(totalEnergyGenerated); Serial.println(" Wh");
  Serial.print("📉 Total Energy Consumed: "); Serial.print(totalEnergyConsumed); Serial.println(" Wh");

  // Send Data to ThingSpeak
  ThingSpeak.setField(1, temperature);  // Field 1: Temperature
  ThingSpeak.setField(2, humidity);     // Field 2: Humidity
  ThingSpeak.setField(3, gasPPM);       // Field 3: Gas PPM
  ThingSpeak.setField(4, solarVoltage); // Field 4: Solar Voltage
  ThingSpeak.setField(5, batteryVoltage); // Field 5: Battery Voltage
  ThingSpeak.setField(6, currentDraw * 1000); // Field 6: Current (mA)
  ThingSpeak.setField(7, powerGenerated); // Field 7: Power Generated (W)
  ThingSpeak.setField(8, powerConsumed);  // Field 8: Power Consumed (W)

  int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (responseCode == 200) { 
    Serial.println("\n✅ Data successfully sent to ThingSpeak!");
  } else { 
    Serial.print("\n❌ Error sending data to ThingSpeak. HTTP Response Code: ");
    Serial.println(responseCode);
    // Attempt to reconnect to WiFi if there's an error
    if (WiFi.status() != WL_CONNECTED) {
      connectToWiFi();
    }
  }

  delay(10000); // Wait 10 seconds before the next reading
}