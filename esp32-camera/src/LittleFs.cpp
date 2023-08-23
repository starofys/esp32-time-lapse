#include "esp_inc.h"
#include <pb_decode.h>
#define SD LittleFS
#define DBG_OUTPUT_PORT Serial

void handleFileList(WebServer *server) {
  if (!server->hasArg("dir")) {
    server->send(400, "text/plain", "BAD ARGS");
    return;
  }

  String path = server->arg("dir");
  Serial.println("handleFileList: " + path);


  File root = SD.open(path);
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
File uploadFile;
void handleFileUpload(WebServer *server) {
  HTTPUpload& upload = server->upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (SD.exists((char *)upload.filename.c_str())) {
      SD.remove((char *)upload.filename.c_str());
    }
    if (uploadFile) {
      uploadFile.close();
    }
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    DBG_OUTPUT_PORT.print("Upload: START, filename: "); DBG_OUTPUT_PORT.println(upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
    DBG_OUTPUT_PORT.print("Upload: WRITE, Bytes: "); DBG_OUTPUT_PORT.println(upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    DBG_OUTPUT_PORT.print("Upload: END, Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}
void initConfig(WebServer *server) {
  server->on("/edit", HTTP_POST, [&]() {
    server->send(200, "text/plain", "ok");
  }, [&]() {
    handleFileUpload(server);
  });
}

bool initFs(WebServer *server) {
    if(!LittleFS.begin(false)){
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    Serial.printf("LittleFS.totalBytes %d",LittleFS.totalBytes());
    
    if (!server) {
        return false;
    }

    //SERVER INIT
  //list directory
    server->on("/list", HTTP_GET, [&server](){
        handleFileList(server);
    });

    initConfig(server);

    server->serveStatic("/",LittleFS,"/","max-age=86400");

    return true;

}

static bool _readConfig(const char* path,const pb_msgdesc_t *fields,void* dist){
  File f = LittleFS.open(path,FILE_READ);
  if (!f) {
    return false;
  }
  size_t size = f.size();
  if (size > 512) {
    Serial.println("config file is big");
    return false;
  }
  uint8_t buffer[512];
  pb_istream_t istream = pb_istream_from_buffer(buffer, size);
  return pb_decode(&istream, fields, dist);

}
bool readConfig(const char* path,const pb_msgdesc_t *fields,void* dist){
  bool val = _readConfig(path,fields,dist);
  Serial.printf("read config %s %d\n",path,val);
  return val;
}