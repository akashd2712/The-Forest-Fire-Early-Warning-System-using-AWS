
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "DHT.h"             // Install "DHT sensor library" by Adafruit

// -------- CONFIG ----------
#define WIFI_SSID        "vivo T4 Pro"
#define WIFI_PASSWORD    "123456789"
#define AWS_IOT_ENDPOINT "a2um96y0gptvpm-ats.iot.ap-south-1.amazonaws.com"
#define THINGNAME        "ESP_LED_Device"

// ============================================================================
// AWS IoT CERTIFICATE SETUP - SECURITY IMPORTANT!
// ============================================================================
// DO NOT commit actual certificates to version control!
// Follow these steps to add your own certificates:
//
// 1. Go to AWS IoT Core Console
// 2. Create a new "Thing" or use existing thing
// 3. Generate device certificates (you'll get 3 files):
//    - cert.pem.crt (Device Certificate)
//    - private.pem.key (Private Key)
//    - AmazonRootCA1.pem (CA Certificate)
//
// 4. Copy the certificate contents between the BEGIN/END markers
// 5. Keep private keys secure - never share or commit them
//
// For detailed setup, see README.md
// ============================================================================

static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
[Paste your AmazonRootCA1.pem content here]
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
[Paste your cert.pem.crt content here]
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
[Paste your private.pem.key content here]
-----END RSA PRIVATE KEY-----
)EOF";

// -------- PINS & SENSORS ----------
#define DHTPIN        4
#define DHTTYPE       DHT11
DHT dht(DHTPIN, DHTTYPE);

#define FLAME_PIN     18   // digital output from flame module
#define FLAME_ACTIVE_STATE LOW  // change to HIGH if your module asserts HIGH when flame present

#define MQ3_PIN       34   // analog input (0-4095)
#define MQ3_THRESHOLD 400  // default threshold for "alcohol detected" (tune after calibration)

#define LED_PIN       2  // status LED

// MQTT Topics
const char* SUB_TOPIC = "esp32/led";
const char* TELEMETRY_TOPIC = "esp32/sensor";
const char* ALERT_TOPIC = "esp32/alert";

// ---------- Globals ----------
WiFiClientSecure net;
PubSubClient client(net);

unsigned long lastTelemetry = 0;
const unsigned long TELEMETRY_INTERVAL_MS = 10000; // 10s

// ---------- Function prototypes ----------
void connectAWS();
void messageHandler(char* topic, byte* payload, unsigned int len);
void publishTelemetry(float temp, float hum, int mq3, bool flameDetected);
void publishAlert(const char* alertMessage, float temp, float hum, int mq3, bool flameDetected);
void blinkLED(int times, int tms);

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(FLAME_PIN, INPUT);
  // MQ3 pin is analog input; no pinMode needed for ADC on ESP32

  dht.begin();

  // WiFi + AWS connect
  connectAWS();
}

// ---------- Connect to Wi-Fi and AWS IoT Core ----------
void connectAWS() {
  Serial.print("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int wifiTry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTry < 30) {
    delay(500);
    Serial.print(".");
    wifiTry++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect WiFi - restarting...");
    delay(2000);
    ESP.restart();
  }
  Serial.println("\nWiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  // Setup TLS certificates
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);

  Serial.print("Connecting to AWS IoT...");
  while (!client.connected()) {
    if (client.connect(THINGNAME)) break;
    Serial.print(".");
    delay(1000);
  }
  if (client.connected()) {
    Serial.println("\nConnected to AWS IoT Core!");
    client.subscribe(SUB_TOPIC);
    Serial.printf("Subscribed to %s\n", SUB_TOPIC);
  } else {
    Serial.println("\nFailed to connect MQTT. Restarting...");
    delay(2000);
    ESP.restart();
  }
}

// ---------- MQTT message handler ----------
void messageHandler(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  msg.trim();
  msg.toUpperCase();
  Serial.print("MQTT msg on "); Serial.print(topic); Serial.print(": "); Serial.println(msg);

  if (msg == "ON") {
    digitalWrite(LED_PIN, HIGH);
    client.publish(TELEMETRY_TOPIC, "LED_ON");
  } else if (msg == "OFF") {
    digitalWrite(LED_PIN, LOW);
    client.publish(TELEMETRY_TOPIC, "LED_OFF");
  } else if (msg == "BLINK") {
    blinkLED(5, 200);
    client.publish(TELEMETRY_TOPIC, "LED_BLINK");
  } else {
    Serial.println("Unknown command");
  }
}

// ---------- Publish telemetry (JSON) ----------
void publishTelemetry(float temp, float hum, int mq3, bool flameDetected) {
  // Build small JSON by hand (to avoid ArduinoJson dependency)
  char payload[256];
  unsigned long ts = millis();
  int n = snprintf(payload, sizeof(payload),
                   "{\"ts\":%lu,\"temperature\":%.1f,\"humidity\":%.1f,\"mq3\":%d,\"flame\":%s}",
                   ts, temp, hum, mq3, flameDetected ? "true" : "false");
  if (n > 0) {
    client.publish(TELEMETRY_TOPIC, payload);
    Serial.print("Published telemetry: "); Serial.println(payload);
  }
}

// ---------- Publish alert (JSON) ----------
void publishAlert(const char* alertMessage, float temp, float hum, int mq3, bool flameDetected) {
  char payload[384];
  unsigned long ts = millis();
  int n = snprintf(payload, sizeof(payload),
                   "{\"ts\":%lu,\"alert\":\"%s\",\"temperature\":%.1f,\"humidity\":%.1f,\"mq3\":%d,\"flame\":%s}",
                   ts, alertMessage, temp, hum, mq3, flameDetected ? "true" : "false");
  if (n > 0) {
    client.publish(ALERT_TOPIC, payload);
    Serial.print("Published ALERT: "); Serial.println(payload);
  }
}

// ---------- Blink LED helper ----------
void blinkLED(int times, int tms) {
  for (int i = 0; i < times; ++i) {
    digitalWrite(LED_PIN, HIGH);
    delay(tms);
    digitalWrite(LED_PIN, LOW);
    delay(tms);
  }
}

// ---------- Main loop ----------
void loop() {
  if (!client.connected()) {
    Serial.println("MQTT disconnected. Reconnecting...");
    connectAWS();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastTelemetry > TELEMETRY_INTERVAL_MS) {
    lastTelemetry = now;

    // Read DHT
    float h = dht.readHumidity();
    float t = dht.readTemperature(); // Celsius
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      // still continue with other sensors
    }

    // Read MQ-3 (analog)
    int mq3 = analogRead(MQ3_PIN); // 0 - 4095 on ESP32 ADC default
    // Note: For better ADC stability, you may want to use analogSetAttenuation and averaging

    // Read flame digital
    int flameRaw = digitalRead(FLAME_PIN);
    bool flameDetected = (flameRaw == FLAME_ACTIVE_STATE);

    // Publish telemetry
    publishTelemetry(t, h, mq3, flameDetected);

    // Evaluate alert conditions
    bool needAlert = false;
    char alertMsg[128] = {0};

    if (flameDetected) {
      snprintf(alertMsg, sizeof(alertMsg), "FLAME DETECTED!");
      needAlert = true;
    } else if (mq3 >= MQ3_THRESHOLD) {
      snprintf(alertMsg, sizeof(alertMsg), "SMOKE DETECTED: %d", mq3);
      needAlert = true;
    }

    if (needAlert) {
      publishAlert(alertMsg, t, h, mq3, flameDetected);
      // Blink LED quickly to indicate alert locally
      blinkLED(10, 100);
    }
  }
}
