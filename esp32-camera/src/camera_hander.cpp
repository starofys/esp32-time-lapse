#include "camera_handler.h"
#include "esp_inc.h"

#include <esp_pm.h>
#include <esp_wifi.h>



extern camera_config_t config;

esp_err_t parse_get(httpd_req_t *req, char **obuf);

esp_pm_config_esp32_t pm_config = {0};

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char *buf = NULL;
    char variable[32];
    char value[32];

    if (parse_get(req, &buf) != ESP_OK) {
        return ESP_FAIL;
    }
    if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) != ESP_OK ||
        httpd_query_key_value(buf, "val", value, sizeof(value)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    free(buf);

    Serial.println(variable);
    Serial.println(value);

    if (!strcmp(variable, "power")) {
        if(!strcmp(value, "1")) {
            // setCpuFrequencyMhz(80);
            esp_wifi_set_ps(WIFI_PS_NONE);
            camera_enable_out_clock(&config);
        } else {
            //setCpuFrequencyMhz(40);
            camera_disable_out_clock();
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        }
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, value, strlen(value));
}

 httpd_uri_t cmd_uri = {
        .uri = "/cus",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
        ,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
#endif
    };

void init_camera_handlers(httpd_handle_t camera_httpd)
{

     httpd_register_uri_handler(camera_httpd, &cmd_uri);
}