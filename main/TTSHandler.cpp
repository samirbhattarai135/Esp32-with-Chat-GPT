
#include <WiFi.h>
#include "TTSHandler.h"
#include <ArduinoJson.h>
#include <BluetoothA2DPSink.h>

const char* serverName = "https://api.openai.com/v1/audio/speech";
const char* apiKey = "Api Key";

WiFiClientSecure client;
HTTPClient http;
BluetoothA2DPSink a2dp_sink;

uint8_t audio_buffer[1024];
size_t audio_buffer_size = 0;

// Callback function to provide audio data to the Bluetooth speaker
void audio_data_callback(const uint8_t* data, uint32_t len) {
    // Here we directly copy the data to the buffer and update buffer size
    memcpy(audio_buffer, data, len);
    audio_buffer_size = len;
}

//Connecting to wifi
void connectToWiFi(const char* ssid, const char* password) { 
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

bool getTTS(String text, String &audioUrl) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi is connected. Sending request to OpenAI TTS...");

    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(apiKey));

    String jsonRequest = "{\"model\":\"tts-1\",\"input\":\"" + text + "\",\"voice\":\"alloy\"}";
    Serial.println("Request JSON: " + jsonRequest);

    int httpResponseCode = http.POST(jsonRequest);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      if (httpResponseCode == 200) {
        Serial.println("Request sent. Processing response...");

        WiFiClient *stream = http.getStreamPtr();
        int available = stream->available();

        while (available) {
        int bytesRead = stream->readBytes(reinterpret_cast<char*>(&audioData[0]), available);
        available -= bytesRead;
        }

        Serial.println("Audio data received.");
        http.end();
        return true;
      } 
      String response = http.getString();
      Serial.println("Response: " + response);

      // Here you would parse the response to extract the audio URL.
      // For example, if the response is JSON:
      StaticJsonDocument<1024> doc;
      deserializeJson(doc, response);

      if (doc.containsKey("audio_url")) {
        audioUrl = doc["audio_url"].as<String>();
        Serial.println("Audio URL: " + audioUrl);
        return true;
      } else {
        Serial.println("Error: No audio URL found in response.");
        return false;
      }
  if (audioData.size() > 0) {
        a2dp_sink.write_data(audioData.data(), audioData.size());
        Serial.println("Audio streaming complete.");
    } else 