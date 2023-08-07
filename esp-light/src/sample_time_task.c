#include "sample_time_task.h"
#include <time.h>



void time_task_delete_task_id(time_task_list_t * q, uint16_t taskId)
{
	time_task_node_t *prev = NULL;
	time_task_node_t *node = q->first;
	while (node) {
		if (node->task->flag&TASK_USER&&taskId == node->id) {
			time_task_list_delete(q,node);
			break;
		}
		else {
			prev = node;
			node = node->next;
			if (node == q->first) {
				break;
			}
		}
	}
}


time_task_t * time_task_new_task(time_task_list_t * q, uint32_t interval, uint8_t flag, funcp_t cb, void * thiz)
{
	time_task_t *task1 = (time_task_t*)malloc(sizeof(time_task_t));
	if (task1) {
		task1->last_time = 0;
		task1->flag = flag;
		task1->interval = interval;
		task1->cb = cb;
		task1->thiz = thiz;
		task1->node = NULL;
		return task1;
	}
	return NULL;
}

time_task_node_t* time_task_list_add(time_task_list_t *q, time_task_t *task)
{
	if (task) {
		return time_task_list_add_at(q, task, NULL, 0);
	}
	return NULL;
}

time_task_node_t* time_task_list_add_at(time_task_list_t *q, time_task_t *task, time_task_node_t *task_node, uint8_t before)
{
	if (!task) {
		return NULL;
	}
	time_task_node_t *newNode = (time_task_node_t*)malloc(sizeof(time_task_node_t));
	if (newNode == NULL)
		return NULL;
	else {

		newNode->id = ++q->taskId;
		task->node = newNode;
		newNode->task = task;

		if (q->first == NULL) {
			q->first = newNode;
		}
		if (q->last == NULL) {
			q->last = newNode;
		}
		if (q->current == NULL) {
			q->current = newNode;
		}

		if (task_node) {
			if (before) {
				task_node->prev->next = newNode;
				newNode->prev = task_node->prev;
				newNode->next = task_node;
				task_node->prev = newNode;
				if (task_node == q->first) {
					q->first = newNode;
				}

			}
			else {
				task_node->next->prev = newNode;
				newNode->prev = task_node;
				newNode->next = task_node->next;
				task_node->next = newNode;

				if (task_node == q->last) {
					q->last = newNode;
				}
			}
		}
		else {

			newNode->prev = q->last;
			newNode->next = q->first;
			q->last->next = newNode;
			q->last = newNode;

			q->first->prev = q->last;
		}

		return newNode;
	}
}

void time_task_list_delete(time_task_list_t *q, time_task_node_t *task_node)
{
	if (!task_node) {
		return;
	}
	task_node->prev->next = task_node->next;
	task_node->next->prev = task_node->prev;
	if (q->current == task_node) {
		q->current = q->current->prev;
		if (q->current == task_node) {
			q->current = NULL;
		}
	}
	if (q->first == task_node) {
		q->first = task_node->next;
		if (q->first == task_node) {
			q->first = NULL;
		}
	}
	if (q->last == task_node) {
		q->last = task_node->prev;
		if (q->last == task_node) {
			q->last = NULL;
		}
	}
	if (task_node->prev->prev == task_node) {
		task_node->prev->prev = task_node->prev;
	}
	//printf("delete task id=%d\n", task_node->id);
	free(task_node->task);
	free(task_node);
}

void time_task_loop(time_task_list_t * tasks, time_t time)
{
	if (tasks->current) {
		do {
			time_task_node_t *current = tasks->current;
			time_task_t *task = current->task;
			if (task->cb && (task->flag&TASK_LOOP_ON)) {
				if ((task->flag&TASK_LOOP_ON) && time - task->last_time >= task->interval) {
					if (task->last_time == 0) {
						if (task->flag&TASK_BEGIN_ADD_ON) {
							//printf("id=%d,now=%d\n", current->id, time);
							task->cb(tasks, current, time);
						}
					}
					else {
						task->cb(tasks, current, time);
					}
					task->last_time = time;
					//if ((!task->flag&TASK_LOOP_ON) && (tasks->current == current)) {
					//	time_task_list_delete(tasks, current);
					//}
				}
			}
			if (tasks->current) {
				tasks->current = tasks->current->next;
			}
		} while (tasks->current&&tasks->current != tasks->first);
	}
}


void timimg_task_loop(time_task_list_t * q, time_task_node_t * _node, time_t timestamp)
{
	//printf("timimg_task_loop\n");
	timing_task_list_t *tasks= _node->task->thiz;
	if (tasks&&tasks->first) {
		time_t now;
		struct tm tm;
		time(&now);
#ifdef _MSC_VER
		localtime_s(&tm, &now);
#else
		tm = *localtime(&now);
#endif
		timing_task_node_t *node = tasks->first;
		//printf("wday=%d,tm_hour=%d,tm_min=%d,tm_sec=%d\n",tm.tm_wday, tm.tm_hour, tm.tm_min,tm.tm_sec);
		while (node)
		{
			
			if (node->flag&TASK_LOOP_ON) {
				//最小的时间窗口为1s 如果在1s时间之内没有tick则可能会导致定时任务不执行

				if (
					((node->timing.detail.week & 0x80) || ((node->timing.detail.week >> tm.tm_wday) & 0x01)) &&
					(node->timing.detail.hour & 0x80 || node->timing.detail.hour == tm.tm_hour) &&
					(node->timing.detail.min & 0x80 || node->timing.detail.min == tm.tm_min)
					&& (node->timing.detail.sec & 0x80 || node->timing.detail.sec == tm.tm_sec))
				{
					node->cb(tasks, node, now);
					if (node->flag&TASK_LOOP_ONCE) {
						timing_task_node_t *next = node->next;
						time_task_timing_task_delete(tasks, node);
						node = next;
						continue;
					}
					//if (node->timing.sec & 0x80) {
					//	node->cb(tasks, node, now);
					//}else{
					//	if ((node->flag & 0x01) == 0 && tm.tm_sec-node->timing.sec<3) {
					//		node->cb(tasks, node, now);
					//		node->flag |= 0x01;
					//		node = node->next;
					//		continue;
					//	}
					//	if (tm.tm_sec - node->timing.sec>3) {
					//		node->flag &= ~0x01;
					//	}
					//}
				}
			}
			node = node->next;
		}
	}
}

timing_task_node_t * time_task_new_timing_task(timing_task_list_t *q, timing_task_info_t *info, uint8_t flag, funcp_timing_t cb, void * thiz)
{
	timing_task_node_t *newNode = (timing_task_node_t*)malloc(sizeof(timing_task_node_t));
	if (newNode == NULL)
		return NULL;
	newNode->timing = *info;
	newNode->cb = cb;
	newNode->flag = flag;
	newNode->thiz = thiz;
	q->taskId++;
	newNode->id = q->taskId;
	newNode->next = NULL;
	if (!q->first) {
		q->first = newNode;
	}
	if (q->last) {
		q->last->next = newNode;
	}
	q->last = newNode;
	return newNode;
}

void time_task_timing_task_delete(timing_task_list_t * q, timing_task_node_t * task_node)
{
	timing_task_node_t *prev = NULL;
	timing_task_node_t *node = q->first;
	while (node) {
		if (task_node == node) {
			if (prev) {
				prev->next = node->next;
			}
			if (q->first == node) {
				q->first = NULL;
			}
			if (q->last == node) {
				q->last = NULL;
			}
			free(node);
			break;
		}
		else {
			prev = node;
			node = node->next;
		}
		
	}
}
void time_task_delete_timing_id(timing_task_list_t * q, uint16_t taskId)
{
	timing_task_node_t *prev = NULL;
	timing_task_node_t *node = q->first;
	while (node) {
		if (node->flag&TASK_USER&&taskId == node->id) {
			if (prev) {
				prev->next = node->next;
			}
			if (q->first == node) {
				q->first = NULL;
			}
			if (q->last == node) {
				q->last = NULL;
			}
			free(node);
			break;
		}
		else {
			prev = node;
			node = node->next;
		}
	}
}
