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
function call_with_id(id)
    str = "call with id: " .. id 
    fib = fib(id)
    str1 = str .. " fib: " .. fib .. "\r\n"
    print(str1)
end