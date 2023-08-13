#include "esp_inc.h"

void handleFileList(WebServer *server) {
  if (!server->hasArg("dir")) {
    server->send(400, "text/plain", "BAD ARGS");
    return;
  }

  String path = server->arg("dir");
  Serial.println("handleFileList: " + path);


  File root = LittleFS.open(path);
  path = String();

  String output = "[";
  if(root.isDirectory()){
      File file = root.openNextFile();
      while(file){
          if (output != "[") {
            output += ',';
          }
          output += "{\"type\":\"";
          output += (file.isDirectory()) ? "dir" : "file";
          output += "\",\"name\":\"";
          output += String(file.path()).substring(1);
          output += "\"}";
          file = root.openNextFile();
      }
  }
  output += "]";
  server->send(200, "text/json", output);
}

void initFs(WebServer *server) {
    if(!LittleFS.begin(false)){
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.printf("LittleFS.totalBytes %d\n",LittleFS.totalBytes());
    
    if (!server) {
        return;
    }

    //SERVER INIT
  //list directory
    server->on("/list", HTTP_GET, [&server](){
        handleFileList(server);
    });

    server->serveStatic("/",LittleFS,"/","max-age=86400");

}
