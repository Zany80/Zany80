function log(x)
	x = "["..identifier.."] "..	x
	for i = 1, depth() do x = "\t"..x end
	print(x)
end

function bind(func, ...)
	local args={...}
	return function(...)
		local args2 = {...}
		return func(unpack(args), unpack(args2))
	end
end
