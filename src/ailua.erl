-module(ailua).
-on_load(init/0).

-export([
         new/0,
         new/1,
         dofile/2,
         call/3,
         gencall/3,
         process_args/1
    ]).

-define(APPNAME,ailua).
-define(LIBNAME,ailua).
-define(MAX_UINT64,18446744073709551615).
-define(MAX_MINUS_INT64,-9223372036854775808).

call(L,FunName,Args)->
	NewArgs = process_list(Args,[]),
	gencall(L,FunName,NewArgs).

init() ->
  LibName =
  	case code:priv_dir(?APPNAME) of
    	{error,bad_name} ->
            case filelib:is_dir(filename:join(["..",priv])) of
            	true -> filename:join(["..",priv,?LIBNAME]);
            	_ -> filename:join([priv,?LIBNAME])
            end;
        Dir -> filename:join(Dir,?LIBNAME)
    end,
  erlang:load_nif(LibName,0).

not_loaded(Line) ->
  exit({not_loaded,[{module,?MODULE},{line,Line}]}).


%% some lua api
new() ->
  not_loaded(?LINE).
new(_Path)->
	not_loaded(?LINE).

dofile(_L,_FilePath) ->
  not_loaded(?LINE).

gencall(_L,_Func,_InArgs) ->
  not_loaded(?LINE).


process_args(Args)-> process_list(Args,[]).

process_list([],Acc)->lists:reverse(Acc);
process_list([H|T],Acc) when erlang:is_integer(H)->
	H0 = process_item(H),
	process_list(T,[H0|Acc]);
process_list([H|T],Acc) when erlang:is_list(H)->
	H0 = process_item(H),
	process_list(T,[H0|Acc]);
process_list([H|T],Acc) when erlang:is_map(H)->
	H0 = process_item(H),
process_list(T,[H0|Acc]);
process_list([H|T],Acc)-> process_list(T,[H|Acc]).

process_item(Item) when erlang:is_integer(Item)->
	if 
		Item > ?MAX_UINT64 -> Item * 1.0;
		Item < ?MAX_MINUS_INT64 -> Item * 1.0;
		true ->Item
	end;
process_item(Item) when erlang:is_list(Item)->
	case io_lib:printable_list(Item)  of 
		true -> Item;
		false -> process_list(Item,[])
	end;
process_item(Item) when erlang:is_map(Item)->
	M = maps:to_list(Item),
	lists:foldl(fun({K,V},Acc) -> 
			K0 = process_item(K),
			V0 = process_item(V),
			maps:put(K0,V0,Acc)
		end,#{},M);
process_item(Item)-> Item.

