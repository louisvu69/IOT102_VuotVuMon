#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL69CYH3NHa"
#define BLYNK_TEMPLATE_NAME "Test"
#define BLYNK_AUTH_TOKEN "gxjCTFjTlg3SCUOcc59swIbveyKN0u0Q"

#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#define DHTPIN 25            // DHT11 pin
#define DHTTYPE DHT11        // DHT type
#define MQ135_SENSOR_PIN 35  // MQ135 on ADC1 pin
#define BUZZER_PIN 26        // Buzzer pin

WiFiManager wm;
DHT dht(DHTPIN, DHTTYPE);
Adafruit_LiquidCrystal lcd(19, 23, 18, 17, 16, 15);
BlynkTimer timer;

#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64

char ssid[MAX_SSID_LEN];
char password[MAX_PASS_LEN];

float humidity;
float temperature;
String air_quality_label;
bool buzzerState = false;
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;

String interpret_air_quality(int sensor_value) {
  String quality;
  if (sensor_value < 1000) {
    quality = "Dangerous";
  } else if (sensor_value < 2000) {
    quality = "Poor";
  } else if (sensor_value < 3000) {
    quality = "Moderate";
  } else {
    quality = "Good";
  }
  // Switch-case to set buzzerState based on air quality
  switch (quality[0]) {
    case 'G':  // Good
    case 'M':  // Moderate
      buzzerState = false;
      break;
    case 'P':  // Poor
    case 'D':  // Dangerous
      buzzerState = true;
      break;
    default:
      buzzerState = false;  // Fallback
      break;
  }
  return quality;
}

void pushData() {
  Blynk.virtualWrite(V0, air_quality_label);
  Blynk.virtualWrite(V1, String(temperature));
  Blynk.virtualWrite(V2, String(humidity) + " %");
  Blynk.virtualWrite(V3, buzzerState);
  if (buzzerState) {
    String reason = temperature > 40 ? "High Temperature (" + String(temperature) + "°C)" : "Poor Air Quality (" + air_quality_label + ")";
    Blynk.logEvent("air_quality_-_temperature_alert", "Alert: " + reason);
  }
}

void buzzerControl() {
  if (buzzerState && !buzzerActive) {
    buzzerActive = true;
    buzzerStartTime = millis();
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on buzzer
  }
  if (buzzerActive && (millis() - buzzerStartTime >= 3000)) {
    digitalWrite(BUZZER_PIN, LOW);  // Stop buzzer after 3 seconds
    buzzerActive = false;
  }
}

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);  // Set ADC to 0–3.6V range
  pinMode(MQ135_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is off initially
  bool res;
  res = wm.autoConnect("IOT102-Air-Quality", "");
  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } else {
    Serial.println("connected...yeey :)");
    strcpy(ssid, wm.getWiFiSSID(res).c_str());
    strcpy(password, wm.getWiFiPass(res).c_str());
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  }
  lcd.begin(16, 2);
  dht.begin();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" IOT102 Project");
  delay(4000);  // Allow sensor warm-up
  timer.setInterval(1200L, pushData);       // Update Blynk every 1s
  timer.setInterval(1000L, buzzerControl);  // Check buzzer frequently
}

void loop() {
  Blynk.run();
  timer.run();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  int sensor_value = analogRead(MQ135_SENSOR_PIN);
  air_quality_label = interpret_air_quality(sensor_value);
  // Override buzzerState if temperature exceeds 40°C
  if (temperature > 40 && !isnan(temperature)) {
    buzzerState = true;
  }
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%, MQ135 Raw: ");
  Serial.print(sensor_value);
  Serial.print(", Air Quality: ");
  Serial.print(air_quality_label);
  Serial.print(", Buzzer State: ");
  Serial.println(buzzerState);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  if (isnan(temperature)) {
    lcd.print("Err");
  } else {
    lcd.print(temperature, 1);
    lcd.print((char)0xDF);  // Degree symbol
    lcd.print("C");
  }
  lcd.print(" H:");
  if (isnan(humidity)) {
    lcd.print("Err");
  } else {
    lcd.print(humidity, 1);
    lcd.print("%");
  }
  lcd.setCursor(0, 1);
  lcd.print("Air: ");
  lcd.print(air_quality_label);
  delay(500);
}
