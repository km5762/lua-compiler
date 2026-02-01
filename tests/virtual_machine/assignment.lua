x = 5
assert(x == 5, "x == 5")

y = x
assert(y == x, "y == x")
assert(y == 5, "y == 5")

y = 2
assert(y == 2, "y == 2")
assert(x == 5, "x == 5")

z = x + y
assert(z == 7, "z == 7")
