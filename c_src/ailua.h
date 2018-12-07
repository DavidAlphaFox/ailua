#ifndef __AI_LUA_H__
#define __AI_LUA_H__
#include "erl_nif.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

typedef struct
{
    int binding;
    int stop;
    int count;
    lua_State* L;
} ai_lua_t;


ErlNifResourceType *RES_AILUA;
ERL_NIF_TERM atom_ok;
ERL_NIF_TERM atom_error;
ERL_NIF_TERM atom_ailua;
ERL_NIF_TERM atom_undefined;
ERL_NIF_TERM atom_nil;
ERL_NIF_TERM atom_true;
ERL_NIF_TERM atom_false;

void ailua_check_stop(ai_lua_t* ailua);
ERL_NIF_TERM ailua_dofile(ErlNifEnv* env, ai_lua_t* ailua, const ERL_NIF_TERM arg);
ERL_NIF_TERM ailua_call(ErlNifEnv *env, ai_lua_t* ailua,
        const ERL_NIF_TERM arg_func,
        const ERL_NIF_TERM arg_fmt,
        const ERL_NIF_TERM arg_list);
ERL_NIF_TERM make_error_tuple(ErlNifEnv *env, const char *reason);

#endif