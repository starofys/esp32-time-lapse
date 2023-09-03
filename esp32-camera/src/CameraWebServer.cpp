#include "esp_inc.h"
#include <camera_handler.h>
#include <WiFiUdp.h>
#include <SD.h>
#include "SPIRAM.h"
// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "LEDE";
const char* password = "xxx";
// const char * udpAddress = "192.168.123.116";

const int udpSize = 1460 - 4;

CameraOption options;
UdpServerOption udpServerConfig = {8080,""};
camera_fb_t prev = {0};
char hostString[16];
WebServer httpServer(8080);
HTTPUpdateServer httpUpdater(true);
TaskHandle_t cameraTask;
WiFiUDP udp;

int shared_variable = 0;
SemaphoreHandle_t shared_var_mutex = NULL;

bool isUploading() {
  HTTPUpload &up = httpServer.upload();
  if (&up) {
    return up.status == UPLOAD_FILE_START || up.status == UPLOAD_FILE_WRITE;
  } else {
    return false;
  }
  
}
void sendUdp(camera_fb_t *fb) {
  if (strlen(udpServerConfig.ip) ==0 || udpServerConfig.port == 0) {
    return;
  }
  udp.beginPacket(udpServerConfig.ip, udpServerConfig.port);
  int startIdx = 0, endIdx;
  int idx = 0;
  int32_t head;
  int cmd = 1;
  int flag = 0;
  for (;;)
  {
    endIdx = startIdx + udpSize;
    if (endIdx >= fb->len)
    {
      endIdx = fb->len;
      // 结束标志
      flag = 1;
    }
    head = CRC16((const char *)&fb->buf[startIdx], endIdx - startIdx) & 0xfff;
    head |= idx << 12;
    head |= flag << 24;
    head |= cmd << 28;
    udp.write((uint8_t *)&head, 4);
    for (; startIdx < endIdx; startIdx++)
    {
      udp.write(fb->buf[startIdx]);
    }
    if (startIdx >= fb->len)
    {
      break;
    }
    idx++;
  }
  udp.endPacket();
}
bool reloadConfig() {
    bool result = readConfig("/config_camera.pb",CameraOption_fields,&options);
    result = result || readConfig("/config_udp_server.pb",UdpServerOption_fields,&udpServerConfig);
    return result;
}
void cameraHandler( void *pvParameters ) {
  sensor_t *s = esp_camera_sensor_get();
  for(;;) {
    camera_stop();
    delay(options.sleep);

    if(!WiFi.isConnected()) {
      continue;
    }

    if (isUploading()) {
      WiFi.setSleep(false);
      continue;
    }
   
    bool led = false;
    if (options.flag & FLAG_LIGHT) {
      led = true;
    } else if (options.flag & FLAG_AUTO_LIGHT) {
      time_t nowtime;
	    time_t now = time(0);
	    tm* p = localtime(&nowtime);
      if (p->tm_hour > 18 || p->tm_hour < 6) {
        led = true;
      }
    }
    if (led) {
      enable_led(led);
    }
    
    camera_start();
    delay(20);
    unsigned long now = millis();
    camera_fb_t *fb = esp_camera_fb_get();
    if (led) {
      enable_led(false);
    }
    camera_stop();
    if (!fb) {
        log_e("Camera capture failed %d",s->pixformat);
        continue;
    }

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
    sendUdp(fb);

    esp_camera_fb_return(fb);
    if (!isUploading()) {
      WiFi.setSleep(true);
    }
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
          ret = udp.begin(WiFi.localIP(),8080);
          Serial.printf("UDP code=%d\n",ret);
          break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          udp.stop();
          break;
      default: break;
    }
}

void initUpgrade() {
  shared_var_mutex = xSemaphoreCreateMutex();
  httpUpdater.setup(&httpServer);
  httpServer.enableCORS(true);
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

  httpServer.on("/reload", HTTP_GET, [](){
    bool rs = reloadConfig();
    httpServer.send(rs?200:500,"text/plain",rs?"success":"fail");
  });
 
}

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  sprintf(hostString, "YSF%06X", ESP.getEfuseMac());
  
  options.frameSize = FrameSize_HD;
  options.freqMHz = 10;
  options.jpegQuality = 4;
 
  options.flag = FLAG_H_MIRROR | FLAG_V_FLIP;
  options.pixFormat = PixFormat_JPEG;

  options.wbModel = WBMode_Auto;
  options.sleep = 5;

  initUpgrade();

  if (initFs(&httpServer)) {
    reloadConfig();
  }
  
  options.sleep = options.sleep * 1000;

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

  int code =  camera_init(&options);

  if (code != ESP_OK) {
    log_e("Camera init failed");
  } else {
    log_e("Camera init success");
  }

  MDNS.begin(hostString);
  httpServer.begin();
  MDNS.addService("http", "tcp", 8080);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", hostString);

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  startCameraTask();
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
  return len;
}



void loop() {

  httpServer.handleClient();
  delay(50);

}
