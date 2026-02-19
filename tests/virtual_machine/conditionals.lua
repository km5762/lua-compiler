x = 0
if true then
	x = 1
end
assert(x == 1, "true branch runs")

x = 0
if false then
	x = 1
end
assert(x == 0, "false branch skipped")

x = 0
if true then
	x = 1
else
	x = 2
end
assert(x == 1, "true branch chosen")

x = 0
if false then
	x = 1
else
	x = 2
end
assert(x == 2, "else branch chosen")

x = 0
if 5 > 3 then
	x = 1
else
	x = 2
end
assert(x == 1, "5 > 3 is true")

x = 0
if 2 > 3 then
	x = 1
else
	x = 2
end
assert(x == 2, "2 > 3 is false")

x = 0
if true then
	if false then
		x = 1
	else
		x = 2
	end
end
assert(x == 2, "inner else runs")

x = 0
if 0 then
	x = 1
else
	x = 2
end
assert(x == 1, "0 is truthy")

x = 0
if nil then
	x = 1
else
	x = 2
end
assert(x == 2, "nil is falsy")
