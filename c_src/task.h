#ifndef __AILUA_TASK_H__
#define __AILUA_TASK_H__
#include "erl_nif.h"

typedef struct _task task_t;
enum _task_type {
    TASK_UNDEFINED,
    TASK_LUA_CALL,
    TASK_LUA_DOFILE
} ;
typedef enum _task_type task_type_t;

task_t* ailua_task_alloc(void);
void ailua_task_free(task_t* task);
void ailua_task_run(task_t* task);
void ailua_task_set_type(task_t* task, task_type_t type);
void ailua_task_set_pid(task_t* task,ErlNifPid pid);
void ailua_task_set_ref(task_t* task,ERL_NIF_TERM ref);
void ailua_task_set_args(task_t* task,ERL_NIF_TERM arg1,ERL_NIF_TERM arg2,ERL_NIF_TERM arg3);
void ailua_task_set_lua(task_t* task,void* res);
#endif