#ifndef __AI_LUA_COMMON_H__
#define __AI_LUA_COMMON_H__

#define STACK_STRING_BUFF 255

#define INVAILD_FILENAME "invalid_filename"
#define STR_NOT_ENOUGHT_MEMORY "not_enough_memory"
#define STR_DO_FILE_ERROR "do_file_error"
#define STR_TASK_ALLOC_FAILED "task_alloc_failed"
#define STR_TASK_BINDING_FAILED "task_binding_failed"
#define FMT_AND_LIST_NO_MATCH  "format and list not match"
#define FMT_AND_RET_NO_MATCH  "format and ret not match"
#define FMT_WRONG  "format wrong"


ERL_NIF_TERM atom_ok;
ERL_NIF_TERM atom_error;
ERL_NIF_TERM atom_ailua;
ERL_NIF_TERM atom_undefined;
ERL_NIF_TERM atom_true;
ERL_NIF_TERM atom_false;


ERL_NIF_TERM make_error_tuple(ErlNifEnv *env, const char *reason);

#endif
