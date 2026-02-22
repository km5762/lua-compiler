t = {}

t = { 10, 20, 30 }
assert(t[1] == 10, "array element 1 incorrect")
assert(t[2] == 20, "array element 2 incorrect")
assert(t[3] == 30, "array element 3 incorrect")

t = { 1, 2, 3 }
assert(t[3] == 3, "trailing comma broke array")

t = { a = 5, b = 6 }
assert(t.a == 5, "record field a incorrect")
assert(t.b == 6, "record field b incorrect")

t = { [1] = 99, ["x"] = 42 }
assert(t[1] == 99, "[1] key incorrect")
assert(t["x"] == 42, '["x"] key incorrect')

t = { 7, 8, a = 9, [4] = 100 }
assert(t[1] == 7, "mixed: array part 1 wrong")
assert(t[2] == 8, "mixed: array part 2 wrong")
assert(t.a == 9, "mixed: record field wrong")
assert(t[4] == 100, "mixed: explicit numeric key wrong")

t = { 1, 2, [1] = 50 }
assert(t[1] == 50, "explicit key should overwrite array slot")

t = { inner = { 1, 2, 3 } }
assert(t.inner[2] == 2, "nested table access incorrect")

t = {}
assert(t.nope == nil, "missing string key should be nil")
assert(t[999] == nil, "missing numeric key should be nil")

t = { hello = 123 }
assert(t.hello == 123, "dot access failed")
assert(t["hello"] == 123, "bracket string access failed")
