#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "esp_control.h"

extern "C" {
unsigned short CRC16(const char* data, int length);
}

void initFs(WebServer *server);
int camera_init(CameraOption *options);

void camera_start();
void camera_stop();