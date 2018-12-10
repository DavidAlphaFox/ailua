test = { _version = "0.1.1" }
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
    str = "in_table2: " .. id 
    print(str .. "\r\n")
    t = {"c","d","e","f"}
    table = to_erl_map({[1] = "a",[2] = "b",[3] = t})
    return table
end
