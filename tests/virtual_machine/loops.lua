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

-- Numeric for loops

sum = 0
for i = 1, 5 do
	sum = sum + i
end
assert(sum == 15, "basic numeric for failed")

sum = 0
for i = 1, 5, 2 do
	sum = sum + i
end
assert(sum == 9, "numeric for with step failed")

sum = 0
for i = 5, 1, -2 do
	sum = sum + i
end
assert(sum == 9, "numeric for with negative step failed")

ran = false
for i = 5, 1 do
	ran = true
end
assert(ran == false, "numeric for should not run when start > end with positive step")

count = 0
for i = 3, 3 do
	count = count + 1
end
assert(count == 1, "numeric for single iteration failed")

last_i = 0
for i = 1, 3 do
	i = 100
	last_i = last_i + 1
end
assert(last_i == 3, "loop variable modification affected iteration")

a = 1
b = 3
sum = 0
for i = a, b do
	a = 100
	b = 200
	sum = sum + i
end
assert(sum == 6, "for bounds not evaluated once at loop start")
