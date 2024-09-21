

#include "TTSHandler.h"
#include <ArduinoJson.h>
#include <BluetoothA2DPSink.h>

const char* serverName = "https://api.openai.com/v1/audio/speech";
const char* apiKey = "API KEY";


WiFiClientSecure client;
HTTPClient http;
BluetoothA2DPSink a2dp_sink;

extern uint8_t audio_buffer[1024];
size_t audio_buffer_size = 0;
size_t audio_buffer_index = 0;

// Callback function to provide audio data to the Bluetooth speaker
void audio_data_callback(uint8_t *data, size_t len) {
    if (audio_buffer_index < audio_buffer_size) {
        // Fill data with audio buffer content
        size_t bytes_to_copy = min(len, audio_buffer_size - audio_buffer_index);
        memcpy(data, audio_buffer + audio_buffer_index, bytes_to_copy);
        audio_buffer_index += bytes_to_copy;
    } else {
        memset(data, 0, len);  // If no more data, send silence
    }
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

        // JSON request for TTS
        String jsonRequest = "{\"model\":\"tts-1\",\"input\":\"" + text + "\",\"voice\":\"alloy\"}";
        Serial.println("Request JSON: " + jsonRequest);

        int httpResponseCode = http.POST(jsonRequest);

        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);

            if (httpResponseCode == 200) {
                Serial.println("Request sent. Processing response...");

                // Process the response and read audio data
                WiFiClient *stream = http.getStreamPtr();
                size_t available;
                audio_buffer_index = 0;  // Reset the buffer index

                // Read audio data from the stream and fill the audio buffer
                while ((available = stream->available()) > 0) {
                    int bytesRead = stream->readBytes(audio_buffer, min(available, sizeof(audio_buffer)));
                    if (bytesRead > 0) {
                        audio_buffer_size = bytesRead;
                        Serial.print("Audio data received. Bytes read: ");
                        Serial.println(bytesR}