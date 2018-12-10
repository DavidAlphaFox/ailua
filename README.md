# ailua

使用lua作为Erlang的业务插件，用来处理一些配置性的变化，支持非调度器线程池来执行lua。ailua使用的是nif模式直接内嵌到Erlang的虚拟机中，如果lua代码过于复杂的话造成崩溃或内存泄漏，
则会引起Erlang虚拟机崩溃和内存泄漏


## 依赖

因为lua需要readline这个库，所以需要预先安装readline开发库。

## 线程池规则

最少为1个线程，默认会创建Erlang调度器线程的数量一半新线程作为异步线程池，目前不支持指定线程数量的方法。

## 如何使用

### 创建

    {ok,Ref} = ailua:new()

Ref会绑定一个lua虚拟机，该Ref会随着创建进程的退出而释放    
创建Ref的Erlang进程崩溃后，该Ref的行为如下

- 如果该Ref有异步操作，会在最后一个异步操作后释放该Ref绑定的lua虚拟机
- 如果该Ref没有任何异步操作，则立刻释放该Ref绑定的lua虚拟机


### 加载lua文件

    同步方法
    ok = ailua:dofile(Ref,FileName)

    异步方法
    CallerRef = make_ref(),
    CallerPid = self(),
    ok = ailua:dofile_async(L,CallerRef,CallerPid,FileName)

### 执行lua函数
    
    同步方法
    ok = ailua:call(Ref,FunctionName,Args)

    异步方法
    CallerRef = make_ref(),
    CallerPid = self(),
    ok = ailua:async_call(L,CallerRef,CallerPid,FunctionName,Args)

其中FunctionName为字符串，Arags为列表

## 注意事项

- lua脚本需要使用绝对路径，如果想使用相对路径，需要自己设置lua的package相关路径
- 目前Erlang到lua传值不支持tuple和proplists，不支持Erlang高阶函数传入lua
- Erlang到lua传值，字符串最好使用binary而非list
- Erlang到lua传值，原子值true，false会自动转化成lua的boolean型，nil会自动转化成lua的nil值
- Erlang到lua传值，如果整形大于了18446744073709551615，默认方法会将该整形转化成binary
- lua到Erlang传值，boolean会自动变更为atom，nil会自动变更为atom
- lua表的元表中请勿设置_ERL_MAP为true字段，该字段是ailua用来区分lua回传给Erlang的table是list还是map
- lua回传map的时候，请使用to_erl_map(map)来进行转化
- 目前不支持lua的userdata，function等比较特殊类型
- 请避免在lua脚本中执行print操作，ailua并为处理lua的输出，这些输出会直接输出到stderr上

## 例子
ailua并未附带过多例子，但是仍有一个test.lua来进行测试

执行方式如下
    
    make 
    erl -pa ./ebin

    {ok,Ref} = ailua:new().
    ok = ailua:dofile(Ref,"./test/test.lua").
    ailua:call(Ref,"hi",[#{1 => "hi"}]).
    ailua:call(Ref,"test:in_table",[1]).
    ailua:call(Ref,"test.in_table2",[2]).



