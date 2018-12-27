#include "ailua.h"


static int
is_erl_boxer(lua_State *L, int table, const char *box)
{
	int r = 0;
	if(lua_getmetatable(L,table)){
		lua_pushstring(L,box);
		lua_gettable(L, table + 1);
		r = lua_isboolean(L, -1) && lua_toboolean(L, -1);
		lua_pop(L, 2);
	}
	return r;
}

int 
lua_to_erlang(ErlNifEnv* env,ERL_NIF_TERM* out,lua_State *L, int i)
{
	lua_Number nv = 0.0;
	int64_t i64 = 0;
	uint64_t ui64 = 0;
	switch (lua_type(L, i)) {
	    case LUA_TNIL: 
            *out = atom_undefined;
			break;
	    case LUA_TNUMBER:
			//lua max 53bit integer == 	9007199254740992 under 5.3 ?
			nv =  lua_tonumber(L, i);
			i64 =  lua_tointeger(L, i);
			ui64 = lua_tointeger(L, i);
			// 64bit 整形 -9223372036854775808 ~ 9223372036854775807
			// 64bit 无符号整形 0 ~ 18446744073709551615
		    if (fabs(fabs(nv) - ui64) > 0.0000001 ) {
				*out =  enif_make_double(env,lua_tonumber(L, i));
			}else {
				if (nv < 0){
					*out = enif_make_int64(env,i64);
				}else{
					*out = enif_make_uint64(env,ui64);
				}
				
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
			if(len > 0){
				strncpy(buffer,s,len);
			}
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
						lua_pop(L,2); /* remove key and value and stop */
						return 0;
					}
				}
				*out = term;
			} else {
				int len = lua_objlen(L, i);
				int k = 1;
				ERL_NIF_TERM term = enif_make_list(env,0);
				ERL_NIF_TERM cell;
				for (k = 1; k <= len; k++) {
					lua_rawgeti(L, i, k);
					if(lua_to_erlang(env,&cell,L,lua_gettop(L))){
						term = enif_make_list_cell(env,cell,term);
						lua_pop(L, 1);
					}else{
						lua_pop(L, 1);
						return 0;
					}
				}
				if(!enif_make_reverse_list(env,term,out)){
					return 0;
				}
			}
			break;
		}
	}
	return 1;
}

int
erlang_to_lua(ErlNifEnv* env,ERL_NIF_TERM term,lua_State *L)
{
	char buffer[256];
	if(enif_is_atom(env,term)){
		size_t len = 0;
		enif_get_atom_length(env,term,&len,ERL_NIF_LATIN1);
		len++;
		memset(buffer,0,len);
		enif_get_atom(env,term,buffer,len,ERL_NIF_LATIN1);
		if(strncmp(buffer,"nil",3) == 0){
			lua_pushnil(L);
		}else if(strncmp(buffer,"true",4) == 0){
			lua_pushboolean(L, 1);
		}else if(strncmp(buffer,"false",4) == 0){
			lua_pushboolean(L, 0);
		}else if(strncmp(buffer,"undefined",9) == 0){
			lua_pushnil(L);
		}else if(strncmp(buffer,"null",4) == 0){
			lua_pushnil(L);
		}else {
			lua_pushstring(L, buffer);
		}
	}else if (enif_is_binary(env, term)){
 		ErlNifBinary bin;
		enif_inspect_binary(env,term,&bin);
		lua_pushlstring(L, bin.data, bin.size);
	}else if (enif_is_number(env,term)){
		int in = 0;
		unsigned int ui = 0;
		unsigned long ul = 0;
		int64_t i64 = 0;
		uint64_t ui64 = 0;
		double number = 0.0;
		lua_Number vn = 0.0;
		if(enif_get_int(env,term,&in)){
			lua_pushinteger(L,in);
		}else if(enif_get_uint(env,term,&ui)){
			lua_pushinteger(L,ui);
		}else if(enif_get_int64(env,term,&i64)){
			vn = (lua_Number)i64;
			lua_pushnumber(L,vn);
		}else if(enif_get_ulong(env,term,&ul)){
			vn = (lua_Number)ul;
			lua_pushnumber(L,vn);
		}else if(enif_get_uint64(env,term,&ui64)){
			vn = (lua_Number)ui64;
			lua_pushnumber(L,vn);
		}else if(enif_get_double(env,term,&number)){
			lua_pushnumber(L,number);
		}else {
			return 0;
		}
	}else if(enif_is_list(env,term)){
		// list 永远都不转化成string
		size_t len = 0;
		int i = 0;
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
		// 设置为map
		lua_createtable(L,1,0);
		lua_pushstring(L,"_ERL_MAP");
		lua_pushboolean(L,1);
		lua_rawset(L,-3);
		lua_setmetatable(L,-2);

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