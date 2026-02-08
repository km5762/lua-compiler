x = 5
assert(x == 5, "single assignment: x == 5")

x = 5
y = x
assert(y == 5, "copy assignment: y == 5 after y = x")
y = 2
assert(x == 5, "copy assignment independence: x unchanged after y reassigned")

x, y = 1, 2
assert(x == 1, "multi assignment: x == 1")
assert(y == 2, "multi assignment: y == 2")

x, y = 3, 4
x, y = y, x
assert(x == 4, "swap: x == original y")
assert(y == 3, "swap: y == original x")

a, b, c = 1
assert(a == 1, "partial assignment: a == 1")
assert(b == nil, "partial assignment: b == nil")
assert(c == nil, "partial assignment: c == nil")

x = 1
y = 2
x, y = y, x + y
assert(x == 2, "rhs evaluation before assignment: x == old y")
assert(y == 3, "rhs evaluation before assignment: y == x + y")

x = 1
x, x = 2, 3
assert(x == 3, "duplicate lvalues: last assignment wins")

a, b, c = 1 + 1, 2 + 2, 3 + 3
assert(a == 2, "expression assignment: a == 2")
assert(b == 4, "expression assignment: b == 4")
assert(c == 6, "expression assignment: c == 6")

x = 5
x, y = nil
assert(x == nil, "nil assignment: x == nil")
assert(y == nil, "nil assignment: y == nil")
