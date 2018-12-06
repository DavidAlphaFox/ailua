#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

#include "erl_nif.h"

typedef struct _task_queue task_queue_t;

task_queue_t* task_queue_alloc(void);
void task_queue_free(task_queue_t* queue);
int task_queue_push(task_queue_t* queue,void* item);
void* task_queue_pop(task_queue_t* queue);

#endif
