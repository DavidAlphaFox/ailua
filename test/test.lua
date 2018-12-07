test = { _version = "0.1.1" }
_ERL_MAP = {}
function test.hello(self)
    print("hello from ailua" .. self._version)
end
function hi()
    print("hi there")
end
function fib(n)
    if n < 2 then return 1 end
    return fib(n - 2) + fib(n - 1)
  end
function call_with_id(id,t)
    str = "call with id: " .. id
    --fibn = fib(id)
    --str1 = str .. " fib: " .. fibn .. "\r\n"
    hi()
    for k,v in pairs(t) do
        str1 = tostring(k) .. tostring(v) .. "\r\n"
        print(str1)
    end
    --print(str1)
    return str
end
function test:in_table(id)
    str = self._version .. ":" .. id 
    print(str .. "\r\n")
    return str
end
function test.in_table2(id)
    str = "in_table2:" .. id 
    print(str .. "\r\n")
    return {"a","bc","cd"}
end


function erl_map(t)
    if type(t) == 'table' then
        t[_ERL_MAP] = true
        return t
	else
		error[[bad argument #1 to 'erl_map' (table expected)]]
    end
end