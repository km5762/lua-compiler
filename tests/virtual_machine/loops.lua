-- While loops

i = 0
while i < 5 do
	i = i + 1
end
assert(i == 5, "basic while loop failed")

i = 10
while i < 5 do
	i = i + 1
end
assert(i == 10, "while should not execute when condition false")

i = 0
limit = 3
while i < limit do
	i = i + 1
end
assert(i == 3, "condition not re-evaluated properly")

i = 0
while true do
	i = i + 1
	if i == 4 then
		break
	end
end
assert(i == 4, "break did not exit loop")

outer = 0
inner_total = 0

while outer < 3 do
	outer = outer + 1
	inner = 0
	while inner < 2 do
		inner = inner + 1
		inner_total = inner_total + 1
	end
end

assert(outer == 3, "outer loop incorrect")
assert(inner_total == 6, "nested loop count incorrect")

i = 0
while i < 10 do
	i = i + 2
end
assert(i == 10, "loop with step 2 incorrect")

flag = true
count = 0

while flag do
	count = count + 1
	flag = false
end

assert(count == 1, "truthiness evaluation failed")

x = nil
ran = false
while x do
	ran = true
end
assert(ran == false, "nil should stop while loop")

-- Repeat loops

i = 0
repeat
	i = i + 1
until true

assert(i == 1, "repeat should execute at least once")

i = 0
repeat
	i = i + 1
until i == 5

assert(i == 5, "basic repeat loop failed")

i = 0
limit = 3
repeat
	i = i + 1
until i >= limit

assert(i == 3, "repeat condition not re-evaluated properly")

i = 0
repeat
	i = i + 1
	if i == 4 then
		break
	end
until false

assert(i == 4, "break did not exit repeat loop")

outer = 0
inner_total = 0

repeat
	outer = outer + 1
	inner = 0
	repeat
		inner = inner + 1
		inner_total = inner_total + 1
	until inner == 2
until outer == 3

assert(outer == 3, "outer repeat incorrect")
assert(inner_total == 6, "nested repeat count incorrect")

flag = false
count = 0

repeat
	count = count + 1
	flag = true
until flag

assert(count == 1, "repeat truthiness evaluation failed")

x = false
runs = 0

repeat
	runs = runs + 1
	x = true
until x
