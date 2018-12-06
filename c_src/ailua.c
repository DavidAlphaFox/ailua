
#include "ailua.h"
#include "task.h"
#include "task_pool.h"

#define STACK_STRING_BUFF 255

#define STR_NOT_ENOUGHT_MEMORY "not_enough_memory"
#define STR_DO_FILE_ERROR "do_file_error"
#define FMT_AND_LIST_NO_MATCH  "fmt and list not match"
#define FMT_AND_RET_NO_MATCH  "fmt and ret not match"
#define FMT_WRONG  "format wrong"

struct _ai_lua
{
    int id;
    lua_State *L;

};
lua_State*
ailua_lua(ai_lua_t* ailua)
{
    if(NULL == ailua) return NULL;
    return ailua->L;
}

ERL_NIF_TERM
make_error_tuple(ErlNifEnv *env, const char *reason)
{
    return enif_make_tuple2(env, atom_error, enif_make_string(env, reason, ERL_NIF_LATIN1));
}


void
free_ailua(ErlNifEnv* env, void* obj)
{
    // 释放lua
    ai_lua_t *res = (ai_lua_t *)obj;
    if (NULL == res) return;
    if (NULL != res-> L){
        lua_close(res->L);
    }
}

static int
load(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
    const char* mod = "ailua";
    const char *ailua_res = "ailua_t_res";
    int flags = ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER;
    ErlNifSysInfo sys_info;

    RES_LUA = enif_open_resource_type(env, mod, ailua_res, free_ailua, flags, NULL);
    if(RES_LUA == NULL) return -1;

    atom_ok = enif_make_atom(env, "ok");
    atom_ailua = enif_make_atom(env,"ailua");
    atom_error = enif_make_atom(env, "error");
    atom_undefined = enif_make_atom(env,"undefined");
    enif_system_info(&sys_info,sizeof(ErlNifSysInfo));
    int max_thread_workers =  sys_info.scheduler_threads / 2;
    task_pool_t* pool = pool_alloc(max_thread_workers);
    if(NULL == pool) return -1;
    *priv = (void*) pool;

    return 0;
}


static ERL_NIF_TERM
ailua_new(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM ret;
    lua_State* L;
    ai_lua_t* res;

    L = luaL_newstate();
    if(NULL == L) {
        return enif_make_tuple2(env, atom_error, enif_make_string(env, STR_NOT_ENOUGHT_MEMORY, ERL_NIF_LATIN1));
    }

    luaL_openlibs(L);

    res = enif_alloc_resource(RES_LUA, sizeof(struct _ai_lua));
    if(NULL == res) return enif_make_badarg(env);

    ret = enif_make_resource(env, res);
    enif_release_resource(res);

    res->L = L;

    return enif_make_tuple2(env, atom_ok, ret);
};



ERL_NIF_TERM
ailua_dofile(ErlNifEnv* env, lua_State* L, const ERL_NIF_TERM arg)
{
    char buff_str[STACK_STRING_BUFF];
    int size = enif_get_string(env, arg, buff_str, STACK_STRING_BUFF, ERL_NIF_LATIN1);
    if(size <= 0) {
        return make_error_tuple(env, "invalid_filename");
    }

    if(luaL_dofile(L, buff_str) != LUA_OK) {
        const char *error = lua_tostring(L, -1);
        ERL_NIF_TERM error_tuple = make_error_tuple(env, error);
        lua_pop(L,1);
        return error_tuple;
    }
    return atom_ok;
}

static ERL_NIF_TERM
ailua_dofile_sync(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ai_lua_t* res;

    if(argc != 2) {
        return enif_make_badarg(env);
    }

    // first arg: res
    if(!enif_get_resource(env, argv[0], RES_LUA, (void**) &res)) {
        return enif_make_badarg(env);
    }

    return ailua_dofile(env,res->L, argv[1]);
}


static ERL_NIF_TERM
ailua_dofile_async(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ai_lua_t* res;
    task_t* task;
    ErlNifPid pid;

    if(argc != 4) {
        return enif_make_badarg(env);
    }

    // first arg: res
    if(!enif_get_resource(env, argv[0], RES_LUA, (void**) &res)) {
        return enif_make_badarg(env);
    }

    // ref
    if(!enif_is_ref(env, argv[1])){
        return make_error_tuple(env, "invalid_ref");
    }

    // dest pid
    if(!enif_get_local_pid(env, argv[2], &pid)) {
        return make_error_tuple(env, "invalid_pid");
    }

    task = task_alloc();
    if(NULL == task) {
        return make_error_tuple(env, "task_alloc_failed");
    }
    task_set_type(task,TASK_LUA_DOFILE);
    task_set_ref(task,argv[1]);
    task_set_pid(task,pid);
    task_set_args(task,argv[3],atom_undefined,atom_undefined);
    task_set_lua(task,res);
    return add_to_pool(env, res, task);
}




ERL_NIF_TERM
ailua_call(ErlNifEnv *env, lua_State *L,
        const ERL_NIF_TERM arg_func,
        const ERL_NIF_TERM arg_fmt,
        const ERL_NIF_TERM arg_list)
{
    char buff_str[STACK_STRING_BUFF];
    char buff_fmt[STACK_STRING_BUFF];
    char buff_fun[STACK_STRING_BUFF];
    unsigned input_len=0;
    unsigned output_len=0;

    if(enif_get_string(env, arg_func, buff_fun, STACK_STRING_BUFF, ERL_NIF_LATIN1)<=0){
        return enif_make_badarg(env);
    }

    if(enif_get_string(env, arg_fmt, buff_fmt, STACK_STRING_BUFF, ERL_NIF_LATIN1)<=0){
        return enif_make_badarg(env);
    }

    if(!enif_is_list(env, arg_list)){
        return enif_make_badarg(env);
    }

    input_len = strchr(buff_fmt, ':') - buff_fmt;
    output_len = strlen(buff_fmt) - input_len -1;

    // printf("input args %d output args %d fun %s\n", input_len, output_len, buff_fun);
    ERL_NIF_TERM head,tail,list;
    list=arg_list;

    int i=0, status = 0, ret;
    ERL_NIF_TERM return_list = enif_make_list(env, 0);
    lua_getglobal(L, buff_fun);
    //lua_getglobal(L, "test");
    //lua_getfield(L, -1, buff_fun);
    //lua_pushvalue(L, -2);
    const char *error;

    while(buff_fmt[i]!='\0') {
        // printf("i:%d %c\n", i, buff_fmt[i]);

        if(status==0 && buff_fmt[i]!=':') {
            ret = enif_get_list_cell(env, list, &head, &tail);
            if(!ret) {
                error = FMT_AND_LIST_NO_MATCH;
                goto error;
            }
            list=tail;
        }

        switch(buff_fmt[i]) {
        case ':' :
            status=1;
            if(lua_pcall(L, input_len, output_len,0) != LUA_OK) {
                error = lua_tostring(L, -1);
                lua_pop(L,1);
                return enif_make_tuple2(env, atom_error, enif_make_string(env, error, ERL_NIF_LATIN1));
            }
            //output_len = - 1;
            break;
        case 'i':
            if( status == 0) {
                int input_int;
                ret = enif_get_int(env, head, &input_int);
                // printf("input %d\n", input_int);
                if(!ret) {
                    error = FMT_AND_LIST_NO_MATCH;
                    goto error;
                }

                lua_pushinteger(L, input_int);
            } else if ( status==1 ){
                int isnum;
                int n = lua_tointegerx(L, -1, &isnum);
                if(!isnum){
                    error = FMT_AND_LIST_NO_MATCH;
                    goto error;
                }
                // printf("output %d %d\n", output_len, n);
                return_list = enif_make_list_cell(env, enif_make_int(env, n), return_list);
                lua_pop(L,1);
                output_len--;
            }
            break;
        case 's':
            if( status == 0) {
                ret = enif_get_string(env, head, buff_str, STACK_STRING_BUFF, ERL_NIF_LATIN1);
                if(ret<=0) {
                    error = FMT_AND_LIST_NO_MATCH;
                    goto error;
                }
                lua_pushstring(L, buff_str);

            } else if ( status==1 ) {
                const char *s = lua_tostring(L, -1);
                if (s==NULL) {
                    error = FMT_AND_RET_NO_MATCH;
                    goto error;
                }
                // printf("output %d %s\n", output_len, s);
                return_list = enif_make_list_cell(env, enif_make_string(env, s, ERL_NIF_LATIN1), return_list);
                lua_pop(L,1);
                output_len--;
            }
            break;
            /* case 'd': */
            /*     break; */
            /* case 'b': */
            /*     break; */
        default:
            error = FMT_WRONG;
            goto error;
            break;
        }

        i++;
    }
    return enif_make_tuple2(env, atom_ok, return_list);

 error:
    // printf("in error \n");
    // @fix clean the heap var.
    // before call, pop the call
    if(status ==0 ) {
        lua_pop(L, 1);
    }
    else if(output_len>0) {
        lua_pop(L, output_len);
    }

    return make_error_tuple(env, error);
}

static ERL_NIF_TERM
ailua_gencall_sync(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ai_lua_t *res;

    // first arg: ref
    if(!enif_get_resource(env, argv[0], RES_LUA,(void**) &res)) {
        return enif_make_badarg(env);
    }

    if(!enif_is_list(env, argv[3])) {
        return enif_make_badarg(env);
    }

    return ailua_call(env, res->L, argv[1], argv[2], argv[3]);
}
static ERL_NIF_TERM
ailua_gencall_async(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ai_lua_t* res;
    task_t* task;
    ErlNifPid pid;

    if(argc != 6) {
        return enif_make_badarg(env);
    }

    // first arg: ref
    if(!enif_get_resource(env, argv[0], RES_LUA, (void**) &res)) {
        return enif_make_badarg(env);
    }

    // ref
    if(!enif_is_ref(env, argv[1])) {
        return make_error_tuple(env, "invalid_ref");
    }

    // dest pid
    if(!enif_get_local_pid(env, argv[2], &pid)) {
        return make_error_tuple(env, "invalid_pid");
    }

    // fourth arg: list of input args
    if(!enif_is_list(env, argv[5])) {
        return enif_make_badarg(env);
    }

    task = task_alloc();
    if(NULL == task) {
        return make_error_tuple(env, "task_alloc_failed");
    }
    task_set_type(task,TASK_LUA_CALL);
    task_set_ref(task, argv[1]);
    task_set_pid(task,pid);
    task_set_args(task,argv[3],argv[4],argv[5]);
    task_set_lua(task,res);

    return add_to_pool(env, res, task);
}



static ErlNifFunc nif_funcs[] = {
    {"new", 0, ailua_new},

    {"dofile_sync", 2, ailua_dofile_sync},
    {"dofile_async", 4, ailua_dofile_async},

    {"gencall_sync", 4, ailua_gencall_sync},
    {"gencall_async", 6, ailua_gencall_async}

};

ERL_NIF_INIT(ailua, nif_funcs, &load, NULL, NULL, NULL);
