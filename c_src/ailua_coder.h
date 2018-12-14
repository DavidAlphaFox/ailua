#ifndef __AILUA_CODER_H__
#define __AILUA_CODER_H__
#include "erl_nif.h"
#include "luajit.h"
int lua_to_erlang(ErlNifEnv* env,ERL_NIF_TERM* out,lua_State *L, int i);
int erlang_to_lua(ErlNifEnv* env,ERL_NIF_TERM term,lua_State *L);
#endif