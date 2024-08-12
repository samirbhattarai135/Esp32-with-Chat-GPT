#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiClient.h>
#include <WebServer.h>

const char* ssid = "we";
const char* password = "wewewewe";

WebServer server(80);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Print the IP address
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount file system");
        return;
    }
    
    server.on("/download", HTTP_GET, []() {
        File file = SPIFFS.open("/response.mp3", "r");
        if (!file) {
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(file, "audio/mpeg");
        file.close();
    });

    server.begin();
    Serial.println("Server started");
}

void loop() {
    server.handleClient();
}
