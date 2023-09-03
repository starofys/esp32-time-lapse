#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "esp_control.h"

#ifndef CONFIG_LED_ILLUMINATOR_ENABLED
#define CONFIG_LED_ILLUMINATOR_ENABLED 1
#endif

extern "C" {
unsigned short CRC16(const char* data, int length);
}

bool initFs(WebServer *server);
bool readConfig(const char* path,const pb_msgdesc_t *fields,void* dist);
int camera_init(CameraOption *options);

void camera_start();
void camera_stop();

#if CONFIG_LED_ILLUMINATOR_ENABLED
void enable_led(bool en);
#endif