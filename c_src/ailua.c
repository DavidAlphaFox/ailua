
#include "ailua.h"
#include "task.h"
#include "task_pool.h"
#include "ailua_coder.h"
#include "ailua_atomic.h"

#define STACK_STRING_BUFF 255

#define STR_NOT_ENOUGHT_MEMORY "not_enough_memory"
#define STR_DO_FILE_ERROR "do_file_error"
#define FMT_AND_LIST_NO_MATCH  "fmt and list not match"
#define FMT_AND_RET_NO_MATCH  "fmt and ret not match"
#define FMT_WRONG  "format wrong"


struct _ai_lua_res
{
    ailua_t* ailua;
};
static const char *erl_functions_s = ""
" function to_erl_map(t)"
"	if type(t) == 'table' then"
"       meta = {[\"_ERL_MAP\"] = true}"
"		setmetatable(t,meta)"
"       return t "
"	else"
"		error[[bad argument #1 to 'erl_map' (table expected)]]"
"	end"
" end";

static int 
set_lua_path( lua_State* L, const char* path,size_t path_len )
{
    size_t len = 0;
    lua_getglobal( L, "package" );
    lua_getfield( L, -1, "path" ); // get field "path" from table at top of stack (-1)
    const char *s = lua_tolstring(L, -1, &len); // grab path string from top of stack
    const char *buffer = enif_alloc(path_len + len + 1);
    if(NULL == buffer){
        lua_pop(L,2);
        return 0;
    }
    memset(buffer,0,path_len + len + 1);
    strncpy(buffer,s,len);
    strncpy(buffer + len ,path,path_len);
    lua_pop( L, 1 ); // get rid of the string on the stack we just pushed on line 5
    lua_pushstring( L, buffer); // push the new one
    lua_setfield( L, -2, "path" ); // set the field "path" in table at -2 with value at top of stack
    lua_pop( L, 1 ); // get rid of package table from top of stack
    enif_free(buffer);
    return 1; // all done!
}
static ailua_t*
ailua_alloc()
{
    ailua_t* ailua = enif_alloc(sizeof(ailua_t));
    if(NULL == ailua) return NULL;
    lua_State* L = luaL_newstate();
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE);
    if(NULL == L) {
        enif_free(ailua);
    }
    luaL_openlibs(L);
    if(luaL_dostring(L, erl_functions_s) != 0){
        lua_close(L);
        enif_free(ailua);
        return NULL;
    }
    ailua->L = L;
    ailua->stop = 0;
    ailua->binding = -1;
    return ailua;
}

static ailua_t* 
ailua_init(ailua_t* ailua,char* path,size_t path_len)
{
    if(set_lua_path(ailua->L,path,path_len) == 0){

        lua_close(ailua->L);
        enif_free(ailua);
        return NULL;
    }
    return ailua;
}

void 
ailua_check_stop(ailua_t* ailua)
{
    //此处只有工作线程会执行
    int on_fly = ATOM_DEC(&ailua->count);
    bool stop = ATOM_CAS(&ailua->stop,1,1);
    if(stop){
        if(0 == on_fly){
            if(NULL != ailua->L){
                lua_close(ailua->L);
            }
            
            enif_free(ailua);
        }

    }
}

ERL_NIF_TERM
make_error_tuple(ErlNifEnv *env, const char *reason)
{
    return enif_make_tuple2(env, atom_error, enif_make_string(env, reason, ERL_NIF_LATIN1));
}


static void
free_ailua_res(ErlNifEnv* env, void* obj)
{
    // 释放lua
    // 此刻是竞态，需要处理
    struct _ai_lua_res* res = (struct _ai_lua_res *)obj;
    ailua_t* ailua;
    if (NULL == res) return;
    if (NULL != res-> ailua){
        ailua = res->ailua;
        // 此处不应该发生return，一定是true
        bool stopped = ATOM_CAS(&ailua->stop,0,1);
        assert(stopped == true && "fail on stoping lua");
        if(ATOM_CAS(&ailua->count,0,0)){
            if(NULL != ailua->L){
                lua_close(ailua->L);
            }
            enif_free(ailua);
        }
    }
}

static int
load(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
    const char* mod = "ailua";
    const char *ailua_res = "ailua_t_res";
    int flags = ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER;
    ErlNifSysInfo sys_info;

    RES_AILUA = enif_open_resource_type(env, mod, ailua_res, free_ailua_res, flags, NULL);
    if(RES_AILUA == NULL) return -1;

    atom_ok = enif_make_atom(env, "ok");
    atom_ailua = enif_make_atom(env,"ailua");
    atom_error = enif_make_atom(env, "error");
    atom_undefined = enif_make_atom(env,"undefined");
    atom_true = enif_make_atom(env,"true");
    atom_false = enif_make_atom(env,"false");
    enif_system_info(&sys_info,sizeof(ErlNifSysInfo));
    int max_thread_workers =  sys_info.scheduler_threads / 2;
    if(max_thread_workers < 1) max_thread_workers = 1;
    task_pool_t* pool = pool_alloc(max_thread_workers);
    if(NULL == pool) return -1;
    *priv = (void*) pool;

    return 0;
}


static ERL_NIF_TERM
ailua_new(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM ret;
    struct _ai_lua_res* res;
    ailua_t* ailua = ailua_alloc();
    if(NULL == ailua){
        return enif_make_tuple2(env, atom_error, enif_make_string(env, STR_NOT_ENOUGHT_MEMORY, ERL_NIF_LATIN1));
    }

    res = enif_alloc_resource(RES_AILUA, sizeof(struct _ai_lua_res));
    if(NULL == res) return enif_make_badarg(env);

    ret = enif_make_resource(env, res);
    enif_release_resource(res);

    res->ailua = ailua;

    return enif_make_tuple2(env, atom_ok, ret);
};
static ERL_NIF_TERM
ailua_new_with_path(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    if(argc != 1) {
        return enif_make_badarg(env);
    }
    if(!enif_is_binary(env, argv[0])) {
        return enif_make_badarg(env);
    }
    ERL_NIF_TERM ret;
    struct _ai_lua_res* res;
    ailua_t* ailua = ailua_alloc();
    if(NULL == ailua){
        return enif_make_tuple2(env, atom_error, enif_make_string(env, STR_NOT_ENOUGHT_MEMORY, ERL_NIF_LATIN1));
    }
    ErlNifBinary bin;
	enif_inspect_binary(env,argv[0],&bin);
    ailua = ailua_init(ailua,bin.data,bin.size);
    if(NULL == ailua){
        return enif_make_tuple2(env, atom_error, enif_make_string(env, STR_NOT_ENOUGHT_MEMORY, ERL_NIF_LATIN1));
    }
    res = enif_alloc_resource(RES_AILUA, sizeof(struct _ai_lua_res));
    if(NULL == res) return enif_make_badarg(env);
    ret = enif_make_resource(env, res);
    enif_release_resource(res);

    res->ailua = ailua;

    return enif_make_tuple2(env, atom_ok, ret);

}


ERL_NIF_TERM
ailua_dofile(ErlNifEnv* env, ailua_t* ailua, const ERL_NIF_TERM arg)
{
    lua_State* L = ailua->L;
    char buff_str[STACK_STRING_BUFF];
    int size = enif_get_string(env, arg, buff_str, STACK_STRING_BUFF, ERL_NIF_LATIN1);
    if(size <= 0) {
        return make_error_tuple(env, "invalid_filename");
    }

    if(luaL_dofile(L, buff_str) != 0) {
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
    struct _ai_lua_res* res;

    if(argc != 2) {
        return enif_make_badarg(env);
    }

    // first arg: res
    if(!enif_get_resource(env, argv[0], RES_AILUA, (void**) &res)) {
        return enif_make_badarg(env);
    }

    return ailua_dofile(env,res->ailua, argv[1]);
}


static ERL_NIF_TERM
ailua_dofile_async(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    struct _ai_lua_res* res;
    ailua_t* ailua;
    task_t* task;
    ErlNifPid pid;
    ERL_NIF_TERM term;
    if(argc != 4) {
        return enif_make_badarg(env);
    }

    // first arg: res
    if(!enif_get_resource(env, argv[0], RES_AILUA, (void**) &res)) {
        return enif_make_badarg(env);
    }

    // ref
    if(!enif_is_ref(env, argv[1])){
        return enif_make_badarg(env);
    }

    // dest pid
    if(!enif_get_local_pid(env, argv[2], &pid)) {
        return enif_make_badarg(env);
    }
    ailua = res->ailua;
    task = ailua_task_alloc();
    if(NULL == task) {
        return make_error_tuple(env, "task_alloc_failed");
    }
    ailua_task_set_type(task,TASK_LUA_DOFILE);
    ailua_task_set_ref(task,argv[1]);
    ailua_task_set_pid(task,pid);
    ailua_task_set_args(task,argv[3],0);
    ailua_task_set_lua(task,ailua);
    if(ATOM_CAS(&ailua->stop,0,0)){
        ATOM_INC(&ailua->count);
        term = add_to_pool(env, ailua, task);
        if(term != atom_ok){
            ATOM_DEC(&ailua->count);
        }
        return term;
    }
    
    return make_error_tuple(env, "task_binding_failed");
}




ERL_NIF_TERM
ailua_call(ErlNifEnv *env, ailua_t* ailua,
        const ERL_NIF_TERM arg_func,
        const ERL_NIF_TERM arg_list)
{
    char buff_fun[STACK_STRING_BUFF];
    size_t input_len = 0;
    size_t output_len = 0;
    lua_State* L = ailua->L;
    size_t table_name_length = 0;
    size_t fun_name_length = 0;
    char* token = NULL;
    int invoke_self = 0;
    if(enif_get_string(env, arg_func, buff_fun, STACK_STRING_BUFF, ERL_NIF_LATIN1)<=0){
        return enif_make_badarg(env);
    }

    enif_get_list_length(env,arg_list,&input_len);
    token = strchr(buff_fun, ':');
    if(NULL != token) {
        table_name_length = token - buff_fun;
        invoke_self = 1;
    }else{
        token = strchr(buff_fun, '.');
        if(NULL != token){
            table_name_length = token - buff_fun;
        }
       
    }
    if(table_name_length > 0 ){
        fun_name_length = strlen(buff_fun) - table_name_length -1;
        buff_fun[table_name_length] = 0;
    }

    // printf("input args %d output args %d fun %s\n", input_len, output_len, buff_fun);
    ERL_NIF_TERM return_list = enif_make_list(env, 0);
    ERL_NIF_TERM term;
    ERL_NIF_TERM head,tail,list;
    int i;
    const char *error;
    list = arg_list;
    if(table_name_length > 0){
        const char* tfun = buff_fun + table_name_length + 1;
        lua_getglobal(L,buff_fun);
        if( LUA_TTABLE == lua_type(L,-1)){
            lua_getfield(L, -1, tfun);
            if( LUA_TFUNCTION != lua_type(L,-1)) return enif_make_badarg(env);
            if(invoke_self > 0){
                lua_insert(L, -2);
            }else{
                lua_remove(L,-2);
            }
        }else{
            return enif_make_badarg(env);
        }
    }else {
        lua_getglobal(L, buff_fun);
    }
     //lua_getglobal(L, "test");
    //lua_getfield(L, -1, buff_fun);
    //lua_pushvalue(L, -2);
    if(input_len > 0){
        for(i = 1; i <= input_len; i++){
            if(enif_get_list_cell(env,list,&head,&list)){
                if(0 == erlang_to_lua(env,head,L)){
                    output_len = lua_gettop(L);
                    lua_pop(L, output_len);
                    return enif_make_badarg(env);
                }
            }else {
                output_len = lua_gettop(L);
                lua_pop(L, output_len);
                return enif_make_badarg(env);
            } 
        }
    }
    if(invoke_self > 0) input_len++ ;
    if(lua_pcall(L, input_len, LUA_MULTRET,0) != 0) {
        error = lua_tostring(L, -1);
        lua_pop(L,1);
        return enif_make_tuple2(env, atom_error, enif_make_string(env, error, ERL_NIF_LATIN1));
    }
    output_len = lua_gettop(L);
    if(output_len == 0){
        return atom_ok;
    }else{
        for (i = output_len; i >= 1; i--) {
			lua_to_erlang(env,&term,L,i);
            return_list = enif_make_list_cell(env, term, return_list);
		}
        lua_pop(L, output_len);
        return enif_make_tuple2(env, atom_ok, return_list);
    }
    
    output_len = lua_gettop(L);
    lua_pop(L, output_len);
    return make_error_tuple(env, error);
}

static ERL_NIF_TERM
ailua_gencall_sync(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    struct _ai_lua_res* res;

    // first arg: ref
    if(!enif_get_resource(env, argv[0], RES_AILUA,(void**) &res)) {
        return enif_make_badarg(env);
    }

    if(!enif_is_list(env, argv[2])) {
        return enif_make_badarg(env);
    }

    return ailua_call(env, res->ailua, argv[1], argv[2]);
}
static ERL_NIF_TERM
ailua_gencall_async(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    struct _ai_lua_res* res;
    ailua_t* ailua;
    task_t* task;
    ErlNifPid pid;
    ERL_NIF_TERM term ;
    if(argc != 6) {
        return enif_make_badarg(env);
    }

    // first arg: lua_State
    if(!enif_get_resource(env, argv[0], RES_AILUA, (void**) &res)) {
        return enif_make_badarg(env);
    }

    // ref
    if(!enif_is_ref(env, argv[1])) {
        return enif_make_badarg(env);
    }

    // dest pid
    if(!enif_get_local_pid(env, argv[2], &pid)) {
        return enif_make_badarg(env);
    }

    // fourth arg: list of input args
    if(!enif_is_list(env, argv[4])) {
        return enif_make_badarg(env);
    }
    ailua = res->ailua;
    task = ailua_task_alloc();
    if(NULL == task) {
        return make_error_tuple(env, "task_alloc_failed");
    }
    ailua_task_set_type(task,TASK_LUA_CALL);
    ailua_task_set_ref(task, argv[1]);
    ailua_task_set_pid(task,pid);
    ailua_task_set_args(task,argv[3],argv[4]);
    ailua_task_set_lua(task,ailua);
    if(ATOM_CAS(&ailua->stop,0,0)){
        ATOM_INC(&ailua->count);
        term = add_to_pool(env, ailua, task);
        if(term != atom_ok){
            ATOM_DEC(&ailua->count);
        }
        return term;
    }
    return make_error_tuple(env, "task_binding_failed");
}



static ErlNifFunc nif_funcs[] = {
    {"new", 0, ailua_new},
    {"new",1,ailua_new_with_path},

    {"dofile_sync", 2, ailua_dofile_sync},
    {"dofile_async", 4, ailua_dofile_async},

    {"gencall_sync", 3, ailua_gencall_sync},
    {"gencall_async", 5, ailua_gencall_async}

};

ERL_NIF_INIT(ailua, nif_funcs, &load, NULL, NULL, NULL);
