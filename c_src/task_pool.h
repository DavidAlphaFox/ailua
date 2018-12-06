#ifndef __TASK_POOL_H__
#define __TASK_POOL_H__

#include "erl_nif.h"
typedef struct _task_pool task_pool_t;

task_pool_t* pool_alloc(int max_wokers);
void pool_free(task_pool_t* pool);
ERL_NIF_TERM add_to_pool(ErlNifEnv* env, void* lua, void* task);

#endif