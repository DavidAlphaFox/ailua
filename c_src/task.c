#include "ailua.h"
#include "task.h"

struct _task
{
    ai_lua_t* lua;
    task_type_t type;

    ErlNifEnv* env;
    ERL_NIF_TERM ref;
    ErlNifPid pid;

    ERL_NIF_TERM arg1;
    ERL_NIF_TERM arg2;
    ERL_NIF_TERM arg3;

};
static ERL_NIF_TERM
task_done(task_t* task, ERL_NIF_TERM result)
{
    return enif_make_tuple3(task->env, atom_ailua, task->ref, result);
}

task_t*
task_alloc()
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
    task->ref = 0;

    return task;
}
void 
task_free(task_t* task)
{
    if(NULL != task->env){
        enif_free_env(task->env);
    }
    enif_free(task);
}

ERL_NIF_TERM
run(task_t* task)
{
    lua_State* L = NULL;
    if(NULL != task->lua) L = ailua_lua(task->lua);
    switch(task->type) {
        case TASK_LUA_DOFILE:
            return ailua_dofile(task->env, L, task->arg1);
        case TASK_LUA_CALL:
            return ailua_call(task->env, task->lua->L, task->arg1, task->arg2, task->arg3);
        default:
            return make_error_tuple(task->env, "invalid_command");
    }
}
void 
task_run(task_t* task)
{
    msg = task_done(task, run(task));
    enif_send(NULL, &(task->pid), task->env, msg);
    if(NULL != task->lua) {
        enif_release_resource(task->lua);
    }
    task_free(task);
}
      
void 
task_set_type(task_t* task, task_type_t type)
{
    task->type = type;
}
void 
task_set_pid(task_t* task,ErlNifPid pid)
{
    task->pid = enif_make_pid(task->env,pid);
}
void 
task_set_ref(task_t* task,ERL_NIF_TERM ref)
{
    task->ref = enif_make_copy(task->env, ref);
}
void 
task_set_args(task_t* task,ERL_NIF_TERM arg1,ERL_NIF_TERM arg2,ERL_NIF_TERM arg3)
{
    task->arg1 = arg1;
    task->arg2 = arg2;
    task->arg3 = arg3;
}
void 
task_set_lua(task_t* task,void* res)
{
    task->lua = (ai_lua_t*)res;   
}