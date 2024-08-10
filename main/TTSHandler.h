#ifndef TTS_HANDLER_H
#define TTS_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <BluetoothA2DPSink.h>

extern WiFiClientSecure client;
extern BluetoothA2DPSink a2dp_sink;

void connectToWiFi(const char* ssid, const char* password);
bool getTTS(String text, String &audioUrl);
void streamAudio(String audioUrl);
void setupBluetooth(const char* deviceName);

#endif
