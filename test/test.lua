local base = require "test/base"

test = { _version = "0.1.1" }


function hi(table)
    path = package.path .. "\r\n"
    print("the table is type: " .. type(table) .. "\r\n")
    print("hi there I have path value: \r\n")
    print(path)
    return table
end

function test:in_table(id)
    str =  "test table: " .. self._version .. " : " .. id 
    --return str
    return ""
end
function test.in_table2(id)
    t = {"c","d","e","f"}
    table = to_erl_map({[1] = "a",[2] = "b",[3] = t})
    return table
end

function big_number(num)
    print(num)
    return num
end

function test:put(k,v)
    print(base)
    base:put(k,v)
end