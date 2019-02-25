
local shared = {}
local base = {_base = shared }

function base:put(k,v)
    print(self)
    self._base[k] = v
end
function base:get(k)
    print(self)
    return self._base[k]
end

return base