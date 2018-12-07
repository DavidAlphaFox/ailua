#include "ailua.h"


static int
is_erl_boxer(lua_State *L, int table, const char *box)
{
	int r;
	lua_getglobal(L, box); //得到盒子包装
	lua_gettable(L, table); //
	r = lua_isboolean(L, -1) && lua_toboolean(L, -1);
	lua_pop(L, 1);
	return r;
}

int 
lua_to_erlang(ErlNifEnv* env,ERL_NIF_TERM* out,lua_State *L, int i)
{
	switch (lua_type(L, i)) {
	    case LUA_TNIL: 
            *out = atom_nil;
			break;
	    case LUA_TNUMBER:
		    if (lua_tointeger(L, i) == lua_tonumber(L, i)) {
				*out = enif_make_int(env,lua_tointeger(L, i));
			} else {
				*out =  enif_make_double(env,lua_tonumber(L, i));
			}
			break;
		case LUA_TBOOLEAN:
			if(lua_toboolean(L, i) == 0){
				*out =  atom_false;
			}else{
				*out = atom_true;
			}
			break;
		case LUA_TSTRING: {
			size_t len;
			const char *s = lua_tolstring(L, i, &len);
			unsigned char* buffer =  enif_make_new_binary(env,len,out);
			strncpy(buffer,s,len);
			*out =  enif_make_string_len(env,s,len,ERL_NIF_LATIN1);
			break;
		}
		case LUA_TTABLE: {
			// 只支持map和list
			/* table is in the stack at index 'i' */
			 if (is_erl_boxer(L,i,"_ERL_MAP")){
				ERL_NIF_TERM term = enif_make_new_map(env);
				ERL_NIF_TERM key_term;
				ERL_NIF_TERM val_term;
				lua_pushnil(L);
				while(lua_next(L,i) != 0){
					int key = lua_gettop(L)-1;
					int val = lua_gettop(L);
					if(lua_to_erlang(env, &key_term,L,key) && lua_to_erlang(env,&val_term,L,val)){
						enif_make_map_put(env,term,key_term,val_term,&term);
						lua_pop(L,1); /* remove 'value'; keep 'key' for next iteration */
					}else{
						lua_pop(L,1); /* remove 'value'; keep 'key' for next iteration */
						return 0;
					}
				}
				*out = term;
			} else {
				int len = lua_rawlen(L, i);
				int k = 1;
				ERL_NIF_TERM term;
				ERL_NIF_TERM* arr = enif_alloc(len * sizeof(ERL_NIF_TERM));
				if(NULL == arr) return 0;
				for (k = 1; k <= len; k++) {
					lua_rawgeti(L, i, k);
					if(lua_to_erlang(env,&term,L,lua_gettop(L))){
						arr[k -1 ] = term; 
						lua_pop(L, 1);
					}else{
						lua_pop(L, 1);
						enif_free(arr);
						return 0;
					}
				}
				*out = enif_make_list_from_array(env, arr,len);
				enif_free(arr);
			}
			break;
		}
	}
	return 1;
}

int
erlang_to_lua(ErlNifEnv* env,ERL_NIF_TERM term,lua_State *L)
{
	if(enif_is_atom(env,term)){
		char* s = NULL;
		size_t len = 0;
		enif_get_atom_length(env,term,&len,ERL_NIF_LATIN1);
		len++;
		s = enif_alloc(len * sizeof(char));
		enif_get_atom(env,term,s,len,ERL_NIF_LATIN1);
		if(strcmp(s,"nil") == 0){
			lua_pushnil(L);
		}else if(strcmp(s,"true") == 0){
			lua_pushboolean(L, 1);
		}else if(strcmp(s,"false") == 0){
			lua_pushboolean(L, 0);
		}else{
			lua_pushstring(L, s);
		}
		enif_free(s);
	}else if (enif_is_binary(env, term)){
 		ErlNifBinary bin;
		enif_inspect_binary(env,term,&bin);
		lua_pushlstring(L, bin.data, bin.size);
	}else if (enif_is_number(env,term)){
		unsigned int integer = 0;
		double number = 0.0;
		if(enif_get_int(env,term,&integer)){
			lua_pushinteger(L,integer);
		}else if(enif_get_int64(env,term,&number)){
			lua_pushnumber(L,number);
		}else if(enif_get_double(env,term,&number)){
			lua_pushnumber(L,number);
		}else if(enif_get_uint(env,term,&integer)){
			lua_pushinteger(L,integer);
		}else if(enif_get_uint64(env,term,&number)){
			lua_pushnumber(L,number);
		}else if(enif_get_ulong(env,term,&number)){
			lua_pushnumber(L,number);
		}else {
			return 0;
		}
	}else if(enif_is_list(env,term)){
		size_t len = 0;
		int i = 0;
		ErlNifBinary bin;
		if(enif_inspect_iolist_as_binary(env, term, &bin)){
			lua_pushlstring(L, bin.data, bin.size);
			return 1;
		}
		enif_get_list_length(env,term,&len);
		ERL_NIF_TERM list = term;
		ERL_NIF_TERM head;
		lua_createtable(L, len, 0);
		if(len > 0){
			for (i = 1; i <= len; i++) {
				if(enif_get_list_cell(env,list,&head,&list)){
					if(erlang_to_lua(env,head,L)){
						lua_rawseti(L, -2, i);
					}else{
						return 0;
					}
				}else{
					return 0;
				}
			}
		}
	}else if(enif_is_map(env,term)){
		size_t len = 0;
		ERL_NIF_TERM key, value;
		ErlNifMapIterator iter;
		enif_get_map_size(env, term,&len);
		lua_createtable(L, len, 0);
		enif_map_iterator_create(env, term, &iter, ERL_NIF_MAP_ITERATOR_FIRST);
		while (enif_map_iterator_get_pair(env, &iter, &key, &value)) {
			if(erlang_to_lua(env,key,L) && erlang_to_lua(env,value,L)){
				lua_rawset(L, -3);
    			enif_map_iterator_next(env, &iter);
			}else{
				return 0;
			}
		
		}
		enif_map_iterator_destroy(env, &iter);
	}else{
		return 0;
	}
	return 1;
}