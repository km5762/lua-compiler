function foo(a, b)
  return a + b
end

local function bar(x)
  return x * 2
end

baz = function(y)
  return y - 1
end

local qux = function()
  return 42
end

function mod.util.add(a, b)
  return a + b
end

function obj.member:method(x)
  return self.value + x
end

foo(bar(3), baz(10))

obj:method(5)
