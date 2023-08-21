#include "esp_inc.h"
#include <camera_handler.h>
#include <WiFiUdp.h>
#include <SD.h>
#include "SPIRAM.h"
// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "LEDE";
const char* password = "zwyysfsj";
const char * udpAddress = "192.168.123.116";
const int udpPort = 8080;
const int udpSize = 1460 - 4;

CameraOption options;
camera_fb_t prev = {0};
char hostString[16];
WebServer httpServer(8080);
HTTPUpdateServer httpUpdater(true);
TaskHandle_t cameraTask;
WiFiUDP udp;

int shared_variable = 0;
SemaphoreHandle_t shared_var_mutex = NULL;

void handlerHttp( void *pvParameters ) {
  for(;;) {
    httpServer.handleClient();
    delay(50);
  }
}

void cameraHandler( void *pvParameters ) {
  sensor_t *s = esp_camera_sensor_get();
  for(;;) {
    camera_stop();
    delay(options.sleep);

    if(!WiFi.isConnected()) {
      continue;
    }

    camera_start();
    delay(20);
    unsigned long now = millis();
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        log_e("Camera capture failed %d",s->pixformat);
        continue;
    }
    camera_stop();
    char ts[32];
    snprintf(ts, 32, "%ld.%06ld", fb->timestamp.tv_sec, fb->timestamp.tv_usec);
    // esp_camera_fb_return(fb);
    Serial.printf("tm = %s, duertion %ld, len = %d\n", ts, millis() - now, fb->len);
    WiFi.setSleep(false);
    uint8_t *buf = prev.buf;
    if (prev.len < fb->len) {
      if (buf) {
        free(buf);
      }
      buf = (uint8_t*)ps_malloc(fb->len);
    }
    prev = *fb;
    prev.buf = buf;
    if (prev.buf) {
      int i = 5;
      do {
        if (xSemaphoreTake(shared_var_mutex, portMAX_DELAY) == pdTRUE) {
          memcpy(prev.buf, fb->buf, fb->len);
          xSemaphoreGive(shared_var_mutex);
          break;
        }
        else {
          delay(50);
        }
        i--;
      } while (i > 0);
      if (i == 0) {
        log_e("camera mutex failed");
      }
    } else {
      prev.buf = 0;
    }
    
    delay(100);
    udp.beginPacket(udpAddress,udpPort);
    int startIdx = 0,endIdx;
    int idx = 0;
    int32_t head;
    int cmd = 1;
    int flag = 0;
    for(;;) {
      endIdx = startIdx + udpSize;
      if (endIdx >= fb->len) {
        endIdx =  fb->len;
        // 结束标志
        flag = 1;
      }
      head = CRC16((const char*)&fb->buf[startIdx],endIdx -  startIdx);
      head |= idx << 16;
      head |= flag << 24;
      head |= cmd << 28;
      udp.write((uint8_t*)&head,4);
      for(;startIdx < endIdx;startIdx++) {
        udp.write(fb->buf[startIdx]);
      }
      if (startIdx >=fb->len) {
        break;
      }
      idx++;
    }
    udp.endPacket();

    esp_camera_fb_return(fb);
    WiFi.setSleep(true);
    
  }
}
void startCameraTask() {
  sensor_t *s = esp_camera_sensor_get();
  if (!s) {
    return;
  }
  xTaskCreatePinnedToCore(
    cameraHandler
    ,  "camera"
    ,  2048  // Stack size
    ,  (void*)s  // When no parameter is used, simply pass NULL
    ,  1  // Priority
    ,  &cameraTask // With task handle we will be able to manipulate with this task.
    ,  ARDUINO_RUNNING_CORE // Core on which the task will run
    );
}

void WiFiEvent(WiFiEvent_t event){
    int ret = 0;
    switch(event) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          ret = udp.begin(WiFi.localIP(),udpPort);
          Serial.printf("UDP code=%d\n",ret);
          break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          udp.stop();
          break;
      default: break;
    }
}


void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  sprintf(hostString, "YSF%06X", ESP.getEfuseMac());
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
	WiFi.hostname(hostString);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");


  
  options.frameSize = FrameSize_UXGA;
  options.freqMHz = 10;
  options.jpegQuality = 4;
  options.pixFormat = PixFormat_JPEG;
  options.flag = FLAG_H_MIRROR | FLAG_V_FLIP;

  options.wbModel = WBMode_Auto;
  options.sleep = 5000;

  int code =  camera_init(&options);

  if (code != ESP_OK) {
    log_e("Camera init failed");
  } else {
    log_e("Camera init success");
  }

  shared_var_mutex = xSemaphoreCreateMutex();

  MDNS.begin(hostString);
  httpUpdater.setup(&httpServer);
  initFs(&httpServer);

  httpServer.on("/capture", HTTP_GET, [](){
    int i=5;
    do {
      if(xSemaphoreTake(shared_var_mutex, portMAX_DELAY) == pdTRUE){
        httpServer.send_P(200,"image/jpeg",(const char*)prev.buf,prev.len);
        xSemaphoreGive(shared_var_mutex);
        return;
      } else {
        delay(50);
      }
      i--;
    } while (i > 0);
    httpServer.send(500,"text/plain","mutex error!");
  });

  httpServer.on("/reboot", HTTP_GET, [](){
    httpServer.send(200,"text/plain","success");
    ESP.restart();
  });

  httpServer.begin();
  MDNS.addService("http", "tcp", 8080);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", hostString);

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");


  xTaskCreatePinnedToCore(
    handlerHttp
    ,  "httpd"
    ,  2048  // Stack size
    ,  NULL  // When no parameter is used, simply pass NULL
    ,  1  // Priority
    ,  NULL // With task handle we will be able to manipulate with this task.
    ,  ARDUINO_RUNNING_CORE // Core on which the task will run
    );

  startCameraTask();
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
  return len;
}



void loop() {

  delay(50);

}
