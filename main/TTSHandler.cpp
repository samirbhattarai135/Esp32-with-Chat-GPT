#include "TTSHandler.h"
#include <ArduinoJson.h>

// Constants
constexpr size_t BUFFER_SIZE = 1024;
const char* serverName = "https://api.openai.com/v1/audio/speech";
const char* apiKey = "API KEY";

// Global Variables
WiFiClientSecure client;
HTTPClient http;
BluetoothA2DPSink a2dp_sink;
uint8_t audio_buffer[BUFFER_SIZE];  // Define the audio buffer
size_t audio_buffer_size = 0;
size_t audio_buffer_index = 0;

// Audio data callback for Bluetooth streaming
void audio_data_callback(const uint8_t *data, uint32_t len) {
    if (audio_buffer_index < audio_buffer_size) {
        size_t bytes_to_copy = std::min((size_t)len, audio_buffer_size - audio_buffer_index);
        memcpy((void *)data, audio_buffer + audio_buffer_index, bytes_to_copy);
        audio_buffer_index += bytes_to_copy;
    } else {
        memset((void *)data, 0, len);  // Send silence when no data is available
    }
}

// Connect to Wi-Fi
void connectToWiFi(const char* ssid, const char* password) { 
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi. IP: " + WiFi.localIP().toString());
}

// Setup Bluetooth A2DP sink
void setupBluetooth(const char* deviceName) {
    a2dp_sink.set_stream_reader(audio_data_callback, true);  // Set the audio callback
    a2dp_sink.start(deviceName);  // Start Bluetooth with the specified device name
    Serial.println("Bluetooth A2DP sink started with name: " + String(deviceName));
}

// Fetch Text-to-Speech (TTS) audio
bool getTTS(String text, String &audioUrl) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected.");
        return false;
    }

    Serial.println("Sending request to OpenAI TTS...");
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(apiKey));

    String jsonRequest = "{\"model\":\"tts-1\",\"input\":\"" + text + "\",\"voice\":\"alloy\"}";
    int httpResponseCode = http.POST(jsonRequest);

    if (httpResponseCode != 200) {
        Serial.println("HTTP error: " + String(httpResponseCode));
        http.end();
        return false;
    }

    // Parse JSON response
    String response = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error || !doc.containsKey("audio_url")) {
        Serial.println("Error parsing JSON or missing 'audio_url'.");
        http.end();
        return false;
    }

    audioUrl = doc["audio_url"].as<String>();
    http.end();
    Serial.println("TTS audio URL received: " + audioUrl);
    return true;
}

// Stream audio from a URL

void streamAudio(const String &audioUrl) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Cannot stream audio.");
        return;
    }

    HTTPClient http;
    Serial.println("Streaming audio from URL...");
    http.begin(audioUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode != HTTP_CODE_OK) {
        Serial.println("Failed to fetch audio. HTTP code: " + String(httpResponseCode));
        http.end();
        return;
    }

    WiFiClient *stream = http.getStreamPtr();
    audio_buffer_index = 0;  // Reset the buffer index

    while (size_t available = stream->available()) {
        int bytesRead = stream->readBytes(audio_buffer, std::min(available, BUFFER_SIZE));
        if (bytesRead > 0) {
            audio_buffer_size = bytesRead;
            Serial.println("Buffering audio... Bytes read: " + String(bytesRead));
        }
    }

    Serial.println("Audio streaming completed.");
    http.end();
}