// C 
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
// erlang
#include "erl_nif.h"
// lua
#include "luajit.h"
#include "lauxlib.h"
#include "lualib.h"
// ailua common
#include "ailua_common.h"
#include "ailua_atomic.h"
#include "ailua_coder.h"


typedef struct
{
  lua_State* L;
} ailua_t;


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
_set_lua_path(lua_State* L, const char* path,size_t path_len )
{
  size_t len = 0;
  size_t extra_len = 1;

  if(path_len <= 0){
    return 1;
  }else{
    if(path[0] != ';'){
      extra_len = 2;
    }
  }
  // 获取lua中package.path
  lua_getglobal( L, "package" );
  lua_getfield( L, -1, "path" ); // get field "path" from table at top of stack (-1)

  const char *s = lua_tolstring(L, -1, &len); // grab path string from top of stack
 	const char *buffer = enif_alloc(path_len + len + extra_len);
  // 分配内存失败，宣告失败
  if(NULL == buffer){
    lua_pop(L,2);
    return 0;
  }
  memset(buffer,0,path_len + len + extra_len);
  strncpy(buffer,s,len);
  if(2 == extra_len){
		strncpy(buffer + len ,';',1)	;
    strncpy(buffer + len + 1 ,path,path_len);
  }else{
    strncpy(buffer + len ,path,path_len);
  }

  lua_pop( L, 1 ); // get rid of the string on the stack we just pushed on line 5
  lua_pushstring( L, buffer); // push the new one
  lua_setfield( L, -2, "path" ); // set the field "path" in table at -2 with value at top of stack
  lua_pop( L, 1 ); // get rid of package table from top of stack
  enif_free(buffer);
  return 1; // all done!
}

void
ailua_free(void* context)
{
  ailua_t* ailua = (ailua_t*)context;
  if(NULL != ailua){
    if(NULL != ailua->L){
      lua_close(ailua->L);
    }
    enif_free(ailua);
  }
}

void*
ailua_alloc(const char* path,size_t path_len)
{
  ailua_t* ailua = enif_alloc(sizeof(ailua_t));
  lua_State* L = luaL_newstate();
  luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE);
  if(NULL == ailua) {
    goto error;
  }
  if(NULL == L) {
    goto error;
  }
  luaL_openlibs(L);
  if(luaL_dostring(L, erl_functions_s) != 0){
    goto error;
  }
  if(NULL != path){
    if(0 == _set_lua_path(L,path,path_len)){
      goto error;
    }
  }
  ailua->L = L;
  return (void*)ailua;

error:
  if(NULL != L){
    lua_close(L);
  }
  if(NULL != ailua){
    enif_free(ailua);
  }
  return NULL;
}

ERL_NIF_TERM
ailua_dofile(ErlNifEnv* env,const ERL_NIF_TERM arg,void* context)
{
  ailua_t* ailua = (ailua_t*)context;
  lua_State* L = ailua->L;
  char buff_str[STACK_STRING_BUFF];
  int size = enif_get_string(env, arg, buff_str, STACK_STRING_BUFF, ERL_NIF_LATIN1);
  if(size <= 0) {
    return make_error_tuple(env, INVAILD_FILENAME);
  }

  if(luaL_dofile(L, buff_str) != 0) {
    const char *error = lua_tostring(L, -1);
    ERL_NIF_TERM error_tuple = make_error_tuple(env, error);
    lua_pop(L,1);
    return error_tuple;
  }
  return atom_ok;
}


ERL_NIF_TERM
ailua_call(ErlNifEnv *env,
           const ERL_NIF_TERM arg_func,
           const ERL_NIF_TERM arg_list,
           void* context)
{
  ailua_t* ailua = (ailua_t*)context;
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
  // 检查是否包含":"或者"." 如果包含":"或者"."
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
    // 此处为调用lua table中函数的逻辑
    // 此处直接重用了buff_buf这个buffer
    const char* tfun = buff_fun + table_name_length + 1;
    lua_getglobal(L,buff_fun);
    if( LUA_TTABLE == lua_type(L,-1)){
      lua_getfield(L, -1, tfun);
      if( LUA_TFUNCTION != lua_type(L,-1)){
        return enif_make_badarg(env);
      }
      // 检查是否需要将table自身放置到顶端
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
  if(input_len > 0){
    // 如果参数数量不为0，将参数放入lua的栈
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
  // 如果是包含self,增加参数个数
  if(invoke_self > 0) input_len++ ;
  if(lua_pcall(L, input_len, LUA_MULTRET,0) != 0) {
    error = lua_tostring(L, -1);
    lua_pop(L,1);
    return enif_make_tuple2(env, atom_error, enif_make_string(env, error, ERL_NIF_LATIN1));
  }
  // 获取返回值的个数
  output_len = lua_gettop(L);
  if(output_len == 0){
    return atom_ok;
  }else{
    // 将返回值放到Erlang的堆栈上
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
