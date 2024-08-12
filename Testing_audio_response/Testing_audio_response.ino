#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <BluetoothA2DPSink.h>
#include <vector>
#include <FS.h>
#include <SPIFFS.h>


// Replace with your network credentials
const char* ssid = "we";
const char* password = "wewewewe";
const char* bluetoothDeviceName = "XBS05"; 

// Replace with your OpenAI API key
const char* serverName = "https://api.openai.com/v1/audio/speech";
const char* apiKey = "Api Key";

// Bluetooth settings
BluetoothA2DPSink a2dp_sink;
WiFiClientSecure client;
HTTPClient http;


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

std::vector<uint8_t> audioData;
String audioUrl;

void audio_data_callback(const uint8_t *data, uint32_t length);
void saveAudioToFile(const String &filename);
bool getTTS(String text);
void streamAudio();

bool getTTS(String text) {
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
                unsigned long startTime = millis();
                const unsigned long timeout = 180000; // 3 minutes timeout

                const int bufferSize = 8192; // Increased buffer size
                std::vector<uint8_t> buffer(bufferSize);

                while (stream->connected() || stream->available()) {
                    int available = stream->available();
                    if (available > 0) {
                        int bytesToRead = min(available, bufferSize);
                        int bytesRead = stream->readBytes(reinterpret_cast<char*>(buffer.data()), bytesToRead);
                        audioData.insert(audioData.end(), buffer.begin(), buffer.begin() + bytesRead);

                        // Reset the timeout timer
                        startTime = millis();
                        Serial.print("Bytes read: ");
                        Serial.println(bytesRead);
                    }

                    // Check for timeout
                    if (millis() - startTime > timeout) {
                        Serial.println("Response processing timed out.");
                        http.end();
                        return false;
                    }
                }

                Serial.print("Total audio data received: ");
                Serial.println(audioData.size());
                http.end();
                return true;
            } else {
                Serial.println("Request failed. Check the response body.");
                String response = http.getString();
                Serial.println("Response body:");
                Serial.println(response);
            }
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);

            if (httpResponseCode == -1) {
                Serial.println("Connection failed - Please check SSL setup or connection.");
            } else {
                Serial.println("Unexpected error.");
            }
        }

        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }
    return false;
}

void saveAudioToFile(const String &filename) {
    if (SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
    }

    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create file");
        return;
    }

    file.write(audioData.data(), audioData.size());
    file.close();

    Serial.print("Audio saved to: ");
    Serial.println(filename);

    // Debugging: Check the file size after saving
    file = SPIFFS.open(filename, FILE_READ);
    if (file) {
        Serial.print("Saved file size: ");
        Serial.println(file.size());
        file.close();
    } else {
        Serial.println("Failed to open the saved file for checking size.");
    }
}

void audio_data_callback(const uint8_t *data, uint32_t length) {
    if (!audioData.empty()) {
        uint32_t bytesToWrite = min(length, (uint32_t)audioData.size());
        memcpy((void *)data, audioData.data(), bytesToWrite);
        audioData.erase(audioData.begin(), audioData.begin() + bytesToWrite);
    }
}

void streamAudio() {
    if (!audioData.empty()) {
        a2dp_sink.set_stream_reader(audio_data_callback);
        a2dp_sink.start(bluetoothDeviceName);
        Serial.println("Streaming audio to Bluetooth speaker...");
    } else {
        Serial.println("No audio data available to stream.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Wait for the serial monitor to open

    String prompt = "Hello";

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount file system");
        return;
    }

    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting...");
    }
    Serial.println("Connected to WiFi");


    // Test HTTPS connection
    client.setCACert(root_ca);
    Serial.println("Testing HTTPS connection...");
    if (!client.connect("api.openai.com", 443)) {
        Serial.println("Connection to API failed!");
    } else {
        Serial.println("Connected to api.openai.com!");
    }

    if (getTTS(prompt)) {
        saveAudioToFile("/response.mp3");  // Save the audio to a file
        streamAudio();  // Stream the audio via Bluetooth
    } else {
        Serial.println("Failed to get TTS");
    }

}

void loop() {
    // Add any additional logic for your application here
}
