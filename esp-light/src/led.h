#include "sample_time_task.h"
#ifndef PIN_SWITCH

#define PIN_SWITCH D8
#define PIN_BTN 0
#define SWITCH_GPIO PIN_SWITCH

#define DEBUG 

// #define HOMEKIT_LOG_LEVEL HOMEKIT_LOG_ERROR

#ifdef __cplusplus
extern "C" {
#endif
extern time_task_list_t iot_tasks;
void init_led();
void toggle_led();
void switchLed(bool status);
void changeBright(int bright);
#ifdef __cplusplus
}
#endif

void my_homekit_setup();
void my_homekit_loop();

#endif