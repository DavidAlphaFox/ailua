
// C 
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
// erlang
#include "erl_nif.h"

// ailua common
#include "ailua_common.h"
#include "ailua_atomic.h"
#include "ailua_coder.h"

// ailua engine
#include "ailua.h"



ErlNifResourceType *RES_AILUA;

struct _ailua_res
{
    void* ailua;
};


static ERL_NIF_TERM
ailua_nif_alloc(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM ret;
    struct _ailua_res* res = NULL;
    void* ailua = NULL;
    ErlNifBinary bin;

    if(argc > 1){
      return enif_make_badarg(env);
    }

    res = enif_alloc_resource(RES_AILUA, sizeof(struct _ailua_res));
    if(NULL == res){
      return enif_make_tuple2(env, atom_error, enif_make_string(env, STR_NOT_ENOUGHT_MEMORY, ERL_NIF_LATIN1));
    }

    if(argc == 1){
      if(!enif_is_binary(env, argv[0])) {
        return enif_make_badarg(env);
      }
      enif_inspect_binary(env,argv[0],&bin);
      ailua = ailua_alloc(bin.data,bin.size);

    }else{
      ailua = ailua_alloc(NULL,0);
    }

    if(NULL == ailua){
      return enif_make_tuple2(env, atom_error, enif_make_string(env, STR_NOT_ENOUGHT_MEMORY, ERL_NIF_LATIN1));
    }

    ret = enif_make_resource(env, res);
    enif_release_resource(res);

    res->ailua = ailua;

    return enif_make_tuple2(env, atom_ok, ret);
};

static ERL_NIF_TERM
ailua_nif_dofile(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    struct _ailua_res* res;

    if(argc != 2) {
        return enif_make_badarg(env);
    }

    // first arg: res
    if(!enif_get_resource(env, argv[0], RES_AILUA, (void**) &res)) {
        return enif_make_badarg(env);
    }

    return ailua_dofile(env,argv[1],res->ailua);
}



static ERL_NIF_TERM
ailua_nif_gencall(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    struct _ailua_res* res;

    // first arg: ref
    if(!enif_get_resource(env, argv[0], RES_AILUA,(void**) &res)) {
        return enif_make_badarg(env);
    }

    if(!enif_is_list(env, argv[2])) {
        return enif_make_badarg(env);
    }

    return ailua_call(env, argv[1], argv[2],res->ailua);
}



static ErlNifFunc nif_funcs[] = {
    {"new", 0, ailua_nif_alloc},
    {"new",1,ailua_nif_alloc},
    {"dofile", 2, ailua_nif_dofile,ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"gencall", 3, ailua_nif_gencall,ERL_NIF_DIRTY_JOB_CPU_BOUND}

};



static void
free_ailua_res(ErlNifEnv* env, void* obj)
{
    // 释放lua
    struct _ailua_res* res = (struct _ailua_res *)obj;
    if (NULL == res) return;
    if (NULL != res->ailua){
      ailua_free(res->ailua);
    }
}

static int
load(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
    const char* mod = "ailua";
    const char *ailua_res = "ailua_res";
    int flags = ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER;

    RES_AILUA = enif_open_resource_type(env, mod, ailua_res, free_ailua_res, flags, NULL);
    if(RES_AILUA == NULL) return -1;

    atom_ok = enif_make_atom(env, "ok");
    atom_ailua = enif_make_atom(env,"ailua");
    atom_error = enif_make_atom(env, "error");
    atom_undefined = enif_make_atom(env,"undefined");
    atom_true = enif_make_atom(env,"true");
    atom_false = enif_make_atom(env,"false");

    return 0;
}

static void
unload(ErlNifEnv* env, void* priv)
{
}


ERL_NIF_INIT(ailua, nif_funcs, &load, NULL, NULL, &unload);
