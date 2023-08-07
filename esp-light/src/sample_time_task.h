#pragma once
#ifndef SAMPLE_TIME_TASK_H
#ifdef  __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define TASK_LOOP_ON				1
#define TASK_BEGIN_ADD_ON			1<<1
#define TASK_LOOP_ONCE				1<<2
#define TASK_USER					1<<3

	typedef struct time_task_list_t time_task_list_t;
	typedef struct time_task_node_t time_task_node_t;
	typedef union timing_task_info_t timing_task_info_t;
	typedef struct timing_task_node_t timing_task_node_t;
	typedef struct timing_task_list_t timing_task_list_t;
	typedef void(*funcp_t)(time_task_list_t *q, time_task_node_t *node, time_t timestamp);
	typedef void(*funcp_timing_t)(timing_task_list_t *q, timing_task_node_t *node, time_t timestamp);

	typedef struct {
		uint8_t flag;
		volatile time_t last_time;
		uint32_t interval;
		funcp_t cb;
		time_task_node_t *node;
		void* thiz;
	} time_task_t;

	struct time_task_node_t {
		uint16_t id;
		time_task_t *task;
		time_task_node_t *prev;
		time_task_node_t *next;
	};

	struct time_task_list_t {
		uint16_t taskId;
		time_task_node_t *current;
		time_task_node_t *first;
		time_task_node_t *last;
	};

	union timing_task_info_t {
		struct
		{
			uint8_t hour;
			uint8_t min;
			uint8_t sec;
			uint8_t week;
		} detail;
		int32_t value;
	};
	struct timing_task_node_t {
		uint8_t flag;
		uint16_t id;
		timing_task_info_t timing;
		struct timing_task_node_t *next;
		void* thiz;
		funcp_timing_t cb;
	};
	struct timing_task_list_t {
		uint16_t taskId;
		struct timing_task_node_t *first;
		struct timing_task_node_t *last;
	};

	

	time_task_t* time_task_new_task(time_task_list_t *q, uint32_t interval,uint8_t flag, funcp_t cb, void* thiz);
	time_task_node_t* time_task_list_add(time_task_list_t *q, time_task_t *task);
	time_task_node_t* time_task_list_add_at(time_task_list_t *q, time_task_t *task, time_task_node_t *task_node, uint8_t before);
	void time_task_list_delete(time_task_list_t *q, time_task_node_t *task_node);
	void time_task_loop(time_task_list_t *tasks, time_t timestamp);

	void timimg_task_loop(time_task_list_t *q, time_task_node_t *node, time_t timestamp);
	timing_task_node_t* time_task_new_timing_task(timing_task_list_t *q, timing_task_info_t *info, uint8_t flag, funcp_timing_t cb, void* thiz);
	void time_task_timing_task_delete(timing_task_list_t *q, timing_task_node_t *task_node);


	void time_task_delete_timing_id(timing_task_list_t * q, uint16_t taskId);
	void time_task_delete_task_id(time_task_list_t * q, uint16_t taskId);

#ifdef  __cplusplus
}
#endif
#define SAMPLE_TIME_TASK_H
#endif // SAMPLE_TIME_TASK_H