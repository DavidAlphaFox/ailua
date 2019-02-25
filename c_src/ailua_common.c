// erlang
#include "erl_nif.h"
// ailua common
#include "ailua_common.h"

ERL_NIF_TERM
make_error_tuple(ErlNifEnv *env, const char *reason)
{
  return enif_make_tuple2(env, atom_error, enif_make_string(env, reason, ERL_NIF_LATIN1));
}

