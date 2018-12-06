#ifndef __AI_LUA_H__
#define __AI_LUA_H__
#include "erl_nif.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct _ai_lua ai_lua_t;
ErlNifResourceType *RES_LUA;
ERL_NIF_TERM atom_ok;
ERL_NIF_TERM atom_error;
ERL_NIF_TERM atom_ailua;
ERL_NIF_TERM atom_undefined;

lua_State* ailua_lua(ai_lua_t* lua);
ERL_NIF_TERM ailua_dofile(ErlNifEnv* env, lua_State* L, const ERL_NIF_TERM arg);
ERL_NIF_TERM ailua_call(ErlNifEnv *env, lua_State *L,
        const ERL_NIF_TERM arg_func,
        const ERL_NIF_TERM arg_fmt,
        const ERL_NIF_TERM arg_list);
ERL_NIF_TERM make_error_tuple(ErlNifEnv *env, const char *reason);

#endif