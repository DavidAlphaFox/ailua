-module(ailua).
-on_load(init/0).

-export([
        new/0,
        dofile_sync/2,
        dofile_async/4,
        gencall_sync/4,
        gencall_async/6,
		async_dofile/2,
		async_gencall/4
    ]).

-define(APPNAME,ailua).
-define(LIBNAME,ailua).


wait(Ref, Timeout) ->
  receive
    {ailua, Ref, Res} -> Res
  after Timeout ->
      throw({error, timeout, Ref})
  end.


async_dofile(L,FilePath) ->
  Ref = make_ref(),
  ok = dofile_async(L,Ref,self(),FilePath),
  wait(Ref, 100000).


async_gencall(L,Func,Format,InArgs) ->
  Ref = make_ref(),
  ok = gencall_async(L,Ref,self(),Func,Format,InArgs),
  wait(Ref, 100000).

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

dofile_sync(_L,_FilePath) ->
  not_loaded(?LINE).
dofile_async(_L,_Ref,_Dest,_FilePath) ->
  not_loaded(?LINE).

gencall_sync(_L,_Func,_Format,_InArgs) ->
  not_loaded(?LINE).
gencall_async(_L,_Ref,_Dest,_Func,_Format,_InArgs) ->
  not_loaded(?LINE).
