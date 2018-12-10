test = { _version = "0.1.1" }


function hi(table)
    print("hi there \r\n")
    return table
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
