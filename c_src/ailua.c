#include "erl_nif.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int
load(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
  lua_State *L;
  L = luaL_newstate(); 
  return 0;
}
static ErlNifFunc nif_funcs[] = {
};

ERL_NIF_INIT(ailua, nif_funcs, &load, NULL, NULL, NULL);

