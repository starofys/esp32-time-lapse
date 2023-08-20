#include "esp_http_server.h"
#include <esp_camera.h>
// #include "esp_timer.h"
// #include "esp_camera.h"
// #include "img_converters.h"
// #include "fb_gfx.h"
// #include "esp32-hal-ledc.h"
// #include "sdkconfig.h"
// #include "camera_index.h"

extern "C" {
esp_err_t camera_enable_out_clock(camera_config_t *config);
void camera_disable_out_clock();
}
void init_camera_handlers(httpd_handle_t camera_httpd);