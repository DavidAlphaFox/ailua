#ifndef __AI_LUA_H__
#define __AI_LUA_H__


//释放lua状态机器
void ailua_free(void* context);
//创建新的lua状态机器
void* ailua_alloc(const char* path,size_t path_len);
//执行lua文件
ERL_NIF_TERM
ailua_dofile(ErlNifEnv* env, const ERL_NIF_TERM arg,void* context);
//执行lua函数
ERL_NIF_TERM
ailua_call(ErlNifEnv *env,
					 const ERL_NIF_TERM arg_func,
					 const ERL_NIF_TERM arg_list,
					 void* context);

#endif

