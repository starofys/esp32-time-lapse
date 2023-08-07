#ifdef ARDUINO

#include <Arduino.h>

#include "wifi_info.h"
#include "sample_time_task.h"
#include "led.h"
#include "mqtt.h"


void web_server_init();
void web_server_loop();

void btn_init(time_task_list_t* iot_tasks);

extern "C" {
char hostString[16];
} 

void setup() {
	init_led();
	Serial.begin(115200);
	sprintf(hostString, "YSF%06X", ESP.getChipId());
	WiFi.hostname(hostString);

	wifi_connect(); // in wifi_info.h
	//homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
	my_homekit_setup();
	delay(10);
	Serial.println("init btn");
	btn_init(&iot_tasks);
	web_server_init();
	delay(10);
	mqtt_setup();
	delay(10);
	switchLed(false);
	delay(10);
	Serial.println("setup finish");
	
}
unsigned long time_now;

void loop() {
	time_now = millis();
	my_homekit_loop();
	web_server_loop();
	time_task_loop(&iot_tasks, time_now);
	mqtt_loop();
	
}


#else
#include <stdio.h>
int main(int argc, char **argv)
{
    printf("hello world %s\n", argv[0]);
    return 0;
}
#endif