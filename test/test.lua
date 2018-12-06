test = { _version = "0.1.1" }
function test.hello(self)
    print("hello from ailua" .. self._version)
end
