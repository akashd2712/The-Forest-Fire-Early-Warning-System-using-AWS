#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // OLED library

// ===============================================
// --- CONFIGURATION SECTION ---
// ⚠️  IMPORTANT: Update these with your actual credentials
//     DO NOT commit real passwords to version control!
//     Use environment variables or .env files instead

// 1. WiFi credentials - UPDATE WITH YOUR OWN
const char* ssid = "YOUR_WIFI_SSID";           // Change this to your WiFi name
const char* password = "YOUR_WIFI_PASSWORD";   // Change this to your WiFi password

// 2. Unique Topic ID (must match with your cloud/app configuration)
const char* UNIQUE_TOPIC_ID = "forest-sector-1";

// 3. Sensor Pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ2_PIN 34
#define FLAME_PIN 18

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===============================================

const char* mqtt_server = "broker.emqx.io";
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
long lastMsg = 0;

// --- MQTT Callback Function ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]");
}

// --- Reconnect to MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" | retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // --- Sensor setup ---
  pinMode(FLAME_PIN, INPUT);
  dht.begin();

  // --- WiFi setup ---
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- MQTT setup ---
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // --- OLED setup ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // I2C address 0x3C
    Serial.println("⚠️ OLED not found!");
    for (;;); // Halt if OLED fails
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Forest Fire System");
  display.println("Connecting WiFi...");
  display.display();
  delay(1500);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.println(WiFi.localIP());
  display.display();
  delay(1500);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int gasValue = analogRead(MQ2_PIN);
    int flameValue = digitalRead(FLAME_PIN);

    if (isnan(h) || isnan(t)) {
      Serial.println("⚠️ Failed to read from DHT sensor!");
    } else {
      // MQTT Topics
      String tempTopic = String(UNIQUE_TOPIC_ID) + "/temperature";
      String humTopic = String(UNIQUE_TOPIC_ID) + "/humidity";
      String gasTopic = String(UNIQUE_TOPIC_ID) + "/gas";
      String flameTopic = String(UNIQUE_TOPIC_ID) + "/flame";

      client.publish(tempTopic.c_str(), String(t).c_str());
      client.publish(humTopic.c_str(), String(h).c_str());
      client.publish(gasTopic.c_str(), String(gasValue).c_str());
      client.publish(flameTopic.c_str(), String(flameValue).c_str());

      // Serial Output
      Serial.println("----- SENSOR DATA -----");
      Serial.print("🌡 Temperature: "); Serial.println(t);
      Serial.print("💧 Humidity: "); Serial.println(h);
      Serial.print("🧪 Gas Level: "); Serial.println(gasValue);
      Serial.print("🔥 Flame Detected: ");
      Serial.println(flameValue == LOW ? "YES" : "NO");
      Serial.println("-----------------------\n");

      // --- OLED Display Update ---
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Forest Fire System");
      display.println("------------------");
      display.print("Temp: "); display.print(t); display.println(" C");
      display.print("Hum : "); display.print(h); display.println(" %");
      display.print("Gas : "); display.println(gasValue);
      display.print("Flame: ");
      display.println(flameValue == LOW ? "🔥 YES" : "NO");
      display.display();
    }
  }
}
