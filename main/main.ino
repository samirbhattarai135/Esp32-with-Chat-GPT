#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

#include <Adafruit_SSD1306.h>
#include <splash.h>

#include "TTSHandler.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "we";
const char* password = "wewewewe";
const char* bluetoothDeviceName = "XBS05";  

const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n" \
"VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n" \
"A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n" \
"WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n" \
"IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n" \
"AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n" \
"QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n" \
"HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n" \
"BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n" \
"9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n" \
"p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n" \
"-----END CERTIFICATE-----\n" \
"";


void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  connectToWiFi(ssid, password);
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WiFi Connected");
  display.display();
  delay(2000);
  display.clearDisplay();

   // Test HTTPS connection
  client.setCACert(root_ca);
  Serial.println("Testing HTTPS connection...");
  if (!client.connect("api.openai.com", 443)) {
    Serial.println("Connection to API failed!");
    display.setCursor(0, 0);
    display.print("API connection failed!");
    display.display();
  } else {
    Serial.println("Connected to api.openai.com!");
    display.setCursor(0, 0);
    display.print("Ready to chat!");
    display.display();
    client.stop();
  }

  setupBluetooth(bluetoothDeviceName);  // Setting up Bluetooth for the speaker
}

void loop() {
  if (Serial.available() > 0) {
    String prompt = Serial.readStringUntil('\n');
    Serial.println("Received prompt: " + prompt);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("User: " + prompt);
    display.display();

    String audioUrl;
    if (getTTS("Hello", audioUrl)) {
      Serial.println("Audio URL: " + audioUrl);
      display.setCursor(0, 0);
      display.print("Assistant: :))");
      display.display();
      streamAudio(audioUrl);
    }
  }
}
