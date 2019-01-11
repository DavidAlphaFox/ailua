local base = require "test/base"
test2 = {}
function test2:get(k)
    print(base)
    return base:get(k)
end