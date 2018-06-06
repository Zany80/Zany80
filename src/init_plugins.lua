depth = 1

function log(x)
	x = "["..identifier.."] "..	x
	for i = 1, depth do x = "\t"..x end
	print(x)
end
