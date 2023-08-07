#include <Arduino.h>
#include "led.h"
#include <homekit/characteristics.h>
#include "sample_time_task.h"
#include <arduino_homekit_server.h>

extern "C"
{
	time_task_list_t iot_tasks = {0};
	extern homekit_characteristic_t cha_switch_on;
	extern homekit_characteristic_t cha_bright;
	extern homekit_server_config_t config;
	
	

	void toggle_led()
	{
		switchLed(!cha_switch_on.value.bool_value);
	}
	void init_led()
	{
		pinMode(PIN_SWITCH, OUTPUT);
		analogWriteRange(100);
		digitalWrite(PIN_SWITCH, HIGH);
		cha_switch_on.value.bool_value = false;
	}
	void switchLed(bool status)
	{
		bool notify = cha_switch_on.value.bool_value != status;
		cha_switch_on.value.bool_value = status;
		//
		if (status) {
			analogWrite(PIN_SWITCH, cha_bright.value.int_value);
		} else {
			digitalWrite(PIN_SWITCH, cha_switch_on.value.bool_value ? HIGH : LOW);
		}

		if (notify) {
			Serial.printf("notify status %d\n",status);
			homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
		}
		
	}

	void changeBright(int bright)
	{
		if (bright < 5) {
			bright = 5;
		}
		bool notify = cha_bright.value.int_value != bright;
		cha_bright.value.int_value = bright; //sync the value
		analogWrite(PIN_SWITCH, bright);

		if (!cha_switch_on.value.bool_value) {
			cha_switch_on.value.bool_value = true;
			homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
		}
		
		if (notify) {
			//Serial.printf("notify bright %d\n",bright);
			homekit_characteristic_notify(&cha_bright, cha_bright.value);
		};
	}
}

//==============================
// HomeKit setup and loop
//==============================


static uint32_t next_heap_millis = 0;

//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value) {
	cha_switch_on.value.bool_value = value.bool_value;
	switchLed(value.bool_value);
}
void set_bright(const homekit_value_t v) {
	cha_bright.value.int_value = v.int_value;
    changeBright(v.int_value);
}
void my_homekit_setup() {
	//Add the .setter function to get the switch-event sent from iOS Home APP.
	//The .setter should be added before arduino_homekit_setup.
	//HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
	//Maybe this is a legacy design issue in the original esp-homekit library,
	//and I have no reason to modify this "feature".
	cha_switch_on.setter = cha_switch_on_setter;
	cha_bright.setter = set_bright;

	arduino_homekit_setup(&config);
	
	
	//report the switch value to HomeKit if it is changed (e.g. by a physical button)
	//bool switch_is_on = true/false;
	//cha_switch_on.value.bool_value = switch_is_on;
	//homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop() {
	yield();
	arduino_homekit_loop();
	// const uint32_t t = millis();
	// if (t > next_heap_millis) {
	// 	// show heap info every 5 seconds
	// 	next_heap_millis = t + 5 * 1000;
	// 	Serial.printf("heap: %d, sockets: %d  \n",
	// 			ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	// }
}