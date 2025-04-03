#define BLYNK_TEMPLATE_ID "TMPL6BlN-e85u"
#define BLYNK_TEMPLATE_NAME "ASM"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"  // Your Blynk Authentication Token

#include <DHT.h>       // Library for DHT sensor
#include <ESP8266WiFi.h> // Library for ESP8266 WiFi connection
#include <BlynkSimpleEsp8266.h> // Blynk library for ESP8266

// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";  // Your WiFi name
const char* pass = "YOUR_WIFI_PASSWORD";  // Your WiFi password

// DHT22 setup
#define DHTPIN D2    // Pin D2 is connected to DHT22
#define DHTTYPE DHT22  // DHT22 sensor type
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT object

// Soil moisture sensor
#define SOIL_MOISTURE_PIN A0  // Pin A0 is connected to the soil moisture sensor
#define SOIL_THRESHOLD 600    // Soil moisture threshold (analog value)

// Device control relays
#define PUMP_RELAY D3    // Pin D3 is connected to the pump relay (LOW = ON, HIGH = OFF)
#define LIGHT_RELAY D6   // Pin D6 is connected to the light relay (LOW = OFF, HIGH = ON)
#define BUZZER_RELAY D7  // Pin D7 is connected to the buzzer relay (LOW = OFF, HIGH = ON)

// Blynk virtual pins
#define VIRTUAL_TEMP V0      // Virtual pin V0 for temperature
#define VIRTUAL_HUMIDITY V1  // Virtual pin V1 for humidity
#define VIRTUAL_SOIL V2    // Virtual pin V2 for soil moisture
#define VIRTUAL_BUZZER V3    // Virtual pin V3 for buzzer
#define VIRTUAL_PUMP V4      // Virtual pin V4 for pump
#define VIRTUAL_LIGHT V5     // Virtual pin V5 for light
#define VIRTUAL_MODE V6     // Virtual pin V6 for mode (0 - Manual, 1 - Automatic)

float temperature;  // Variable to store temperature
float humidity;     // Variable to store humidity
int soilMoisture;   // Variable to store soil moisture

// Variable to control data sending time
unsigned long lastUpdate = 0;  // Last data update time
const long interval = 2000;    // Time interval between updates (2000ms = 2 seconds)

bool autoMode = true; // Default mode is automatic

void setup() {
    Serial.begin(115200);     // Initialize serial communication at 115200 baud rate
    WiFi.begin(ssid, pass);  // Connect to WiFi network
    
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) { // Wait for WiFi connection
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!"); // Notify when connection is successful
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // Connect to Blynk
    
    dht.begin();             // Initialize DHT sensor
    pinMode(SOIL_MOISTURE_PIN, INPUT);    // Set soil moisture sensor pin as input

    pinMode(PUMP_RELAY, OUTPUT);   // Set pump relay pin as output
    pinMode(LIGHT_RELAY, OUTPUT);  // Set light relay pin as output
    pinMode(BUZZER_RELAY, OUTPUT); // Set buzzer relay pin as output
    
    // Set initial state for devices
    digitalWrite(PUMP_RELAY, HIGH);    // Turn off pump (LOW to turn on)
    digitalWrite(LIGHT_RELAY, LOW);     // Turn off light (HIGH to turn on)
    digitalWrite(BUZZER_RELAY, LOW);   // Turn off buzzer (HIGH to turn on)
    
    Blynk.virtualWrite(VIRTUAL_MODE, autoMode); // Send initial mode state to Blynk
    delay(2000);
}

void loop() {
    Blynk.run(); // Run Blynk
    if (millis() - lastUpdate >= interval) { // Check if it's time to update
        lastUpdate = millis();          // Update last update time
        sendSensorData();           // Send sensor data
        checkConditions();          // Check conditions

        if (autoMode) {         // If in automatic mode
            controlDevices();     // Control devices
        }
    }
}

void sendSensorData() {
    temperature = dht.readTemperature(); // Read temperature
    humidity = dht.readHumidity();    // Read humidity
    int rawSoil = analogRead(SOIL_MOISTURE_PIN); // Read analog value from soil moisture sensor
    soilMoisture = constrain(map(rawSoil, 1023, 0, 0, 100), 0, 100); // Convert analog value to percentage (0-100) and limit within 0-100
    
    if (isnan(temperature) || isnan(humidity)) { // Check if there's an error reading data
        Serial.println("Error: Failed to read from DHT22!");
        temperature = -1; // Set error value
        humidity = -1;    // Set error value
    }

    Serial.print("Temperature: "); Serial.print(temperature); Serial.println("°C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
    Serial.print("Soil Moisture: "); Serial.println(soilMoisture);
    Serial.println("--------------------------");
    
    Blynk.virtualWrite(VIRTUAL_TEMP, temperature);    // Send temperature to Blynk
    Blynk.virtualWrite(VIRTUAL_HUMIDITY, humidity);   // Send humidity to Blynk
    Blynk.virtualWrite(VIRTUAL_SOIL, soilMoisture);  // Send soil moisture to Blynk
}

void controlDevices() {
    if (autoMode) {  // Only control if in automatic mode
        if (temperature < 20) {
            digitalWrite(LIGHT_RELAY, HIGH);    // Turn on light
            Blynk.virtualWrite(VIRTUAL_LIGHT, 1);  // Update light status on Blynk
        } else {
            digitalWrite(LIGHT_RELAY, LOW);     // Turn off light
            Blynk.virtualWrite(VIRTUAL_LIGHT, 0);  // Update light status on Blynk
        }

        if (soilMoisture < 30) {
            digitalWrite(PUMP_RELAY, LOW);      // Turn on pump
            Blynk.virtualWrite(VIRTUAL_PUMP, 1);   // Update pump status on Blynk
        } else {
            digitalWrite(PUMP_RELAY, HIGH);    // Turn off pump
            Blynk.virtualWrite(VIRTUAL_PUMP, 0);   // Update pump status on Blynk
        }
    }
}

void checkConditions() {
    if (temperature > 30 || soilMoisture < 30) {  // If temperature is too high or soil is too dry
        Serial.println("Warning: Activating Buzzer!");
        digitalWrite(BUZZER_RELAY, HIGH);    // Turn on buzzer
    } else {
        digitalWrite(BUZZER_RELAY, LOW);     // Turn off buzzer
    }
}

BLYNK_WRITE(VIRTUAL_MODE) { // Function called when virtual pin V6 (mode) changes on Blynk
    autoMode = param.asInt(); // Get value from Blynk (0 or 1)
    Serial.print("Mode changed: ");
    Serial.println(autoMode ? "Automatic" : "Manual"); // Print current mode
}

BLYNK_WRITE(VIRTUAL_BUZZER) { // Function called when virtual pin V3 (buzzer) changes on Blynk
    if (!autoMode) { // Only allow manual control
        int state = param.asInt(); // Get buzzer state from Blynk
        digitalWrite(BUZZER_RELAY, state ? LOW : HIGH); // Control buzzer (note logic level)
    }
}

BLYNK_WRITE(VIRTUAL_PUMP) { // Function called when virtual pin V4 (pump) changes on Blynk
    if (!autoMode) { // Only allow manual control
        int state = param.asInt();
        digitalWrite(PUMP_RELAY, state ? LOW : HIGH); // LOW = ON, HIGH = OFF
        Blynk.virtualWrite(VIRTUAL_PUMP, state);         // Update pump status on Blynk
    }
}

BLYNK_WRITE(VIRTUAL_LIGHT) { // Function called when virtual pin V5 (light) changes on Blynk
    if (!autoMode) { // Only allow manual control
        int state = param.asInt();
        digitalWrite(LIGHT_RELAY, state ? HIGH : LOW); // HIGH = ON, LOW = OFF
        Blynk.virtualWrite(VIRTUAL_LIGHT, state);       // Update light status on Blynk
    }
}

