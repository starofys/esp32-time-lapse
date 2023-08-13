/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp32-webupdate.local/update
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>
#include "esp_inc.h"

#ifndef STASSID
#define STASSID "LEDE"
#define STAPSK  "zwyysfsj"
#endif

const char* host = "esp32-webupdate";
const char* ssid = STASSID;
const char* password = STAPSK;

WebServer httpServer(80);
HTTPUpdateServer httpUpdater;
void printLog();
void setup(void) {

  Serial.begin(115200);
  Serial.println();
  printLog();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  MDNS.begin(host);
  if (MDNS.begin("esp32")) {
    Serial.println("mDNS responder started");
  }


  httpUpdater.setup(&httpServer);

  initFs(&httpServer);

  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
}

void loop(void) {
  httpServer.handleClient();
}

void printLog() {

size_t ul;
esp_partition_iterator_t _mypartiterator;
const esp_partition_t *_mypart;
ul = spi_flash_get_chip_size(); Serial.print("Flash chip size: "); Serial.println(ul);
Serial.println("Partition table:");
_mypartiterator = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
if (_mypartiterator) {
	Serial.println("App Partition table:");
	do {
		_mypart = esp_partition_get(_mypartiterator);
		printf("Type: %02x SubType %02x Address 0x%06X Size 0x%06X Encryption %i Label %s\r\n", _mypart->type, _mypart->subtype, _mypart->address, _mypart->size, _mypart->encrypted, _mypart->label);
	} while (_mypartiterator = esp_partition_next(_mypartiterator));
}
esp_partition_iterator_release(_mypartiterator);
_mypartiterator = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
if (_mypartiterator) {
	Serial.println("Data Partition table:");
	do {
		_mypart = esp_partition_get(_mypartiterator);
		printf("Type: %02x SubType %02x Address 0x%06X Size 0x%06X Encryption %i Label %s\r\n", _mypart->type, _mypart->subtype, _mypart->address, _mypart->size, _mypart->encrypted, _mypart->label);
	} while (_mypartiterator = esp_partition_next(_mypartiterator));
}
esp_partition_iterator_release(_mypartiterator);

}