#include <sample_time_task.h>
#include <arduino_homekit_server.h>
#include <Arduino.h>
#include <btn.h>



time_task_t *btn_task;
unsigned long click_time = 0;

extern "C" homekit_characteristic_t cha_switch_on;
extern "C" 

IRAM_ATTR void on_btn_down()
{
	if (btn_task->flag == TASK_LOOP_ON) {
		return;
	}
	click_time=btn_task->last_time = millis();
	btn_task->flag = TASK_LOOP_ON;
	Serial.printf("btn pressed... \n");
}
void btn_click_task(time_task_list_t *q, time_task_node_t *node, time_t timestamp)
{
	if (digitalRead(PIN_BTN)) {
		//attachInterrupt(PIN_BTN, on_btn_down, FALLING);
		btn_task->flag = 0;
		unsigned long total_time = timestamp - click_time;
		if (total_time < 80) {
			Serial.printf("btn press time low... return  =%d ms\n", total_time);
			return;
		}

		Serial.printf("btn press time =%d ms\n", total_time);

        cha_switch_on.value.bool_value = !cha_switch_on.value.bool_value;

	    homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);

        digitalWrite(PIN_SWITCH, cha_switch_on.value.bool_value ? HIGH : LOW);

	}
}

void btn_init(time_task_list_t* iot_tasks)
{

    btn_task = time_task_new_task(iot_tasks, 50, 0, &btn_click_task, 0);
	if (!btn_task) {
        return;
		
	}
    time_task_list_add(iot_tasks, btn_task);
	attachInterrupt(PIN_BTN, on_btn_down, FALLING);
	analogWriteRange(255);
	pinMode(SWITCH_GPIO, OUTPUT);

	
}
