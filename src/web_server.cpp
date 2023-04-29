#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <arduino_homekit_server.h>

File fsUploadFile,mqttConfigFile;
const char *serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit'/></form>";

ESP8266WebServer server(8080);
Ticker reconnectWifi;



void web_server_loop()
{
    if (WiFi.isConnected())
    {
        server.handleClient();
    }
}
void addHeader()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
}
void handlerOptions()
{
    server.sendHeader("Access-Control-Max-Age", "10000");
    addHeader();
    server.send(200, "text/plain", "");
}
void handler_result(bool success)
{
    server.send(200, "application/json", success ? "{\"success\":true}" : "{\"success\":false}");
}
void handleReset()
{
    Serial.println("homekit reset hahaha\n");
    homekit_storage_reset();
    addHeader();
	handler_result(true);


    reconnectWifi.once(10, []
                               {  SPIFFS.end(),ESP.restart(); });
}
void handleFileUpload() {
	String filename;
	HTTPUpload& upload = server.upload();
	if (upload.totalSize > 102400) {
		return;
	}
	if (upload.status == UPLOAD_FILE_START) {
		
		if (server.uri() == "/config") {
			filename = "";
			filename +="/";
			filename += "config_";
			// filename += MAIN_app->app_info.defineId;
			filename += ".bin";
		}
		else {
			filename = upload.filename;
		}
		if (!filename.startsWith("/")) {
			filename = "/" + filename;
		}
		printf("handleFileUpload Name: %s\n", filename.c_str());
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();
	}
	else if (upload.status == UPLOAD_FILE_WRITE) {
		printf("handleFileUpload Data: %d\n", upload.currentSize);
		if (fsUploadFile) {
			fsUploadFile.write(upload.buf, upload.currentSize);
		}
	}
	else if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile) {
			fsUploadFile.close();
			if (server.uri() == "/config") {
			}
		}
		printf("handleFileUpload Size: %d\n", upload.totalSize);
		
	}
}

void web_server_init()
{
    Serial.println("web_server_init\n");
    SPIFFS.begin();

    if (SPIFFS.exists("/wifi.html"))
    {
        server.serveStatic("/", SPIFFS, "/wifi.html");
    }
    else
    {
        server.on("/", HTTP_GET, []()
                  {
			addHeader();
			server.send(200, "text/html", serverIndex); });
    }
    server.on(
        "/reset", HTTP_GET, handleReset);

    server.on(
        "/upload", HTTP_POST, []()
        {
		addHeader();
		handler_result(true); },
        handleFileUpload);
    server.on(
        "/update", HTTP_POST, []()
        {
            server.sendHeader("Connection", "close");
            addHeader();
            handler_result(!Update.hasError());

            reconnectWifi.once(2, []
                               { ESP.restart(); });
        },
        []()
        {
            HTTPUpload &upload = server.upload();
            if (upload.status == UPLOAD_FILE_START)
            {
                Serial.setDebugOutput(true);
                WiFiUDP::stopAll();
                Serial.printf("Update: %s\n", upload.filename.c_str());
                uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                if (!Update.begin(maxSketchSpace))
                { // start with max available size
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                if (Update.end(true))
                { // true to set the size to the current progress
                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                }
                else
                {
                    Update.printError(Serial);
                }
                Serial.setDebugOutput(false);
            }
            yield();
        });




    server.begin();
}