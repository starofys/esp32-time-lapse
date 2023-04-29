#ifdef ARDUINO

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
#include <sample_time_task.h>
#include <btn.h>

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);


//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_switch_on;
extern "C" char hostString[16];
static uint32_t next_heap_millis = 0;



//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value) {
	bool on = value.bool_value;
	cha_switch_on.value.bool_value = on;	//sync the value
	LOG_D("Switch: %s", on ? "ON" : "OFF");
	digitalWrite(PIN_SWITCH, on ? HIGH : LOW);
}

void my_homekit_setup() {
	pinMode(PIN_SWITCH, OUTPUT);
	

	//Add the .setter function to get the switch-event sent from iOS Home APP.
	//The .setter should be added before arduino_homekit_setup.
	//HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
	//Maybe this is a legacy design issue in the original esp-homekit library,
	//and I have no reason to modify this "feature".
	cha_switch_on.setter = cha_switch_on_setter;
	arduino_homekit_setup(&config);
	digitalWrite(PIN_SWITCH, HIGH);
	//report the switch value to HomeKit if it is changed (e.g. by a physical button)
	//bool switch_is_on = true/false;
	//cha_switch_on.value.bool_value = switch_is_on;
	//homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}
void web_server_init();
void web_server_loop();

void btn_init(time_task_list_t* iot_tasks);

time_task_list_t iot_tasks = { 0 };

void setup() {
	Serial.begin(9600);
	sprintf(hostString, "YSF%06X", ESP.getChipId());
	WiFi.hostname(hostString);

	wifi_connect(); // in wifi_info.h
	//homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
	my_homekit_setup();
	Serial.println("init btn");
	btn_init(&iot_tasks);
	web_server_init();
	Serial.println("init finish");
	
}
unsigned long time_now;

void loop() {
	time_now = millis();
	my_homekit_loop();
	delay(10);
	web_server_loop();

	time_task_loop(&iot_tasks, time_now);
}


#else
#include <stdio.h>
int main(int argc, char **argv)
{
    printf("hello world %s\n", argv[0]);
    return 0;
}
#endif