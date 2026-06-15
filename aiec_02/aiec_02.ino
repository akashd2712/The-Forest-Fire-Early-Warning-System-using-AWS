#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------- CONFIG ----------
#define WIFI_SSID        "vivo T4 Pro"
#define WIFI_PASSWORD    "123456789"
#define AWS_IOT_ENDPOINT "a2um96y0gptvpm-ats.iot.ap-south-1.amazonaws.com"
#define THINGNAME        "ESP_LED_Device"

// -------- OLED CONFIG ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

#define FLAME_PIN     18
#define FLAME_ACTIVE_STATE LOW
#define MQ3_PIN       34
#define MQ3_THRESHOLD 400
#define LED_PIN       2

// MQTT Topics
const char* SUB_TOPIC = "esp32/led";
const char* TELEMETRY_TOPIC = "esp32/sensor";
const char* ALERT_TOPIC = "esp32/alert";

// ---------- Globals ----------
WiFiClientSecure net;
PubSubClient client(net);
unsigned long lastTelemetry = 0;
const unsigned long TELEMETRY_INTERVAL_MS = 10000;

// ---------- Function prototypes ----------
void connectAWS();
void messageHandler(char* topic, byte* payload, unsigned int len);
void publishTelemetry(float temp, float hum, int mq3, bool flameDetected);
void publishAlert(const char* alertMessage, float temp, float hum, int mq3, bool flameDetected);
void blinkLED(int times, int tms);
void updateOLED(float temp, float hum, int mq3, bool flameDetected, const char* status);

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(FLAME_PIN, INPUT);
  dht.begin();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  connectAWS();
}

// ---------- Connect Wi-Fi + AWS ----------
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
    Serial.println("\nWiFi failed. Restarting...");
    delay(2000);
    ESP.restart();
  }
  Serial.println("\nWiFi connected: ");
  Serial.println(WiFi.localIP());

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
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("AWS Connected!");
    display.display();
  } else {
    Serial.println("MQTT failed. Restarting...");
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
  Serial.printf("MQTT msg [%s]: %s\n", topic, msg.c_str());

  if (msg == "ON") {
    digitalWrite(LED_PIN, HIGH);
    client.publish(TELEMETRY_TOPIC, "LED_ON");
  } else if (msg == "OFF") {
    digitalWrite(LED_PIN, LOW);
    client.publish(TELEMETRY_TOPIC, "LED_OFF");
  } else if (msg == "BLINK") {
    blinkLED(5, 200);
    client.publish(TELEMETRY_TOPIC, "LED_BLINK");
  }
}

// ---------- Publish telemetry ----------
void publishTelemetry(float temp, float hum, int mq3, bool flameDetected) {
  char payload[256];
  unsigned long ts = millis();
  snprintf(payload, sizeof(payload),
           "{\"ts\":%lu,\"temperature\":%.1f,\"humidity\":%.1f,\"mq3\":%d,\"flame\":%s}",
           ts, temp, hum, mq3, flameDetected ? "true" : "false");
  client.publish(TELEMETRY_TOPIC, payload);
  Serial.println(payload);
}

// ---------- Publish alert ----------
void publishAlert(const char* alertMessage, float temp, float hum, int mq3, bool flameDetected) {
  char payload[384];
  unsigned long ts = millis();
  snprintf(payload, sizeof(payload),
           "{\"ts\":%lu,\"alert\":\"%s\",\"temperature\":%.1f,\"humidity\":%.1f,\"mq3\":%d,\"flame\":%s}",
           ts, alertMessage, temp, hum, mq3, flameDetected ? "true" : "false");
  client.publish(ALERT_TOPIC, payload);
  Serial.print("ALERT: "); Serial.println(alertMessage);
}

// ---------- Blink LED ----------
void blinkLED(int times, int tms) {
  for (int i = 0; i < times; ++i) {
    digitalWrite(LED_PIN, HIGH);
    delay(tms);
    digitalWrite(LED_PIN, LOW);
    delay(tms);
  }
}

// ---------- OLED Display Update ----------
void updateOLED(float temp, float hum, int mq3, bool flameDetected, const char* status) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Forest Fire Detection ");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.printf("Temp: %.1f C", temp);
  display.setCursor(0, 25);
  display.printf("Hum : %.1f %%", hum);
  display.setCursor(0, 35);
  display.printf("MQ3 : %d", mq3);
  display.setCursor(0, 45);
  display.printf("Flame: %s", flameDetected ? "YES" : "NO");
  display.setCursor(0, 55);
  display.printf("Status: %s", status);

  display.display();
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
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int mq3 = analogRead(MQ3_PIN);
    int flameRaw = digitalRead(FLAME_PIN);
    bool flameDetected = (flameRaw == FLAME_ACTIVE_STATE);

    if (isnan(h) || isnan(t)) {
      Serial.println("DHT read failed!");
    }

    publishTelemetry(t, h, mq3, flameDetected);

    bool needAlert = false;
    char alertMsg[64] = "Normal";
    if (flameDetected) {
      strcpy(alertMsg, "FLAME DETECTED!");
      needAlert = true;
    } else if (mq3 >= MQ3_THRESHOLD) {
      snprintf(alertMsg, sizeof(alertMsg), "SMOKE: %d", mq3);
      needAlert = true;
    }

    updateOLED(t, h, mq3, flameDetected, alertMsg);

    if (needAlert) {
      publishAlert(alertMsg, t, h, mq3, flameDetected);
      blinkLED(10, 100);
    }
  }
}
