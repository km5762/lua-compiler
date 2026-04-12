function factorial(n)
	if n == 0 then
		return 1 -- Base case
	else
		return n * factorial(n - 1) -- Recursive step
	end
end

print(factorial(8))
