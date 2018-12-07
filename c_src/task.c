#include "ailua.h"
#include "task.h"

struct _task
{
    ai_lua_t* ailua;
    task_type_t type;

    ErlNifEnv* env;
    ERL_NIF_TERM ref;
    ERL_NIF_TERM pid;

    ERL_NIF_TERM arg1;
    ERL_NIF_TERM arg2;

};
static ERL_NIF_TERM
task_done(task_t* task, ERL_NIF_TERM result)
{
    return enif_make_tuple3(task->env, atom_ailua, task->ref, result);
}

task_t*
ailua_task_alloc(void)
{
    ErlNifEnv* env;
    task_t* task;

    env = enif_alloc_env();
    if(NULL == env) {
        return NULL;
    }

    task = (task_t *) enif_alloc(sizeof(struct _task));
    if(NULL == task) {
        enif_free_env(env);
        return NULL;
    }

    task->env = env;
    task->type = TASK_UNDEFINED;
    task->pid = 0;
    task->ref = 0;
    task->arg1 = 0;
    task->arg2 = 0;
    return task;
}
void 
ailua_task_free(task_t* task)
{
    if(NULL != task->env){
        enif_free_env(task->env);
    }
    enif_free(task);
}

ERL_NIF_TERM
do_task(task_t* task)
{
    if(NULL == task->ailua) return make_error_tuple(task->env, "invalid_context");
    switch(task->type) {
        case TASK_LUA_DOFILE:
            return ailua_dofile(task->env, task->ailua, task->arg1);
        case TASK_LUA_CALL:
            return ailua_call(task->env, task->ailua, task->arg1, task->arg2);
        default:
            return make_error_tuple(task->env, "invalid_command");
    }
}
void 
ailua_task_run(task_t* task)
{
    ERL_NIF_TERM msg = task_done(task, do_task(task));
    ErlNifPid pid;
    if(0 != task->pid){
        enif_get_local_pid(task->env, task->pid, &pid);
        enif_send(NULL, &pid, task->env, msg);
    }
    ailua_check_stop(task->ailua);
    ailua_task_free(task);
}
      
void 
ailua_task_set_type(task_t* task, task_type_t type)
{
    task->type = type;
}
void 
ailua_task_set_pid(task_t* task,ErlNifPid pid)
{
    task->pid = enif_make_pid(task->env,&pid);
}
void 
ailua_task_set_ref(task_t* task,ERL_NIF_TERM ref)
{
    task->ref = enif_make_copy(task->env, ref);
}
void 
ailua_task_set_args(task_t* task,ERL_NIF_TERM arg1,ERL_NIF_TERM arg2)
{
    if(0 != arg1){
        task->arg1 = enif_make_copy(task->env,arg1);
    }
    if(0 != arg2){
        task->arg2 = enif_make_copy(task->env,arg2);
    }    
}
void 
ailua_task_set_lua(task_t* task,void* res)
{
    task->ailua = (ai_lua_t*)res;
}