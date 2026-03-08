function add(a, b)
	return a + b
end

local a = 1
local b = 2
assert((a + b) == add(a, b), "add function")

print(add(4, 2))
