cpus = {}
executables = {}

depth = 1

plugin_functions = {}
function plugin_functions:enumerate()
	for k, v in pairs(self) do
		if type(v) == "table" and not v.enumerated then
			self[#self + 1] = v v.enumerated = k
		end
	end
end

setmetatable(cpus, {__index = plugin_functions })
setmetatable(executables, {__index = plugin_functions })

function bind(func, ...)
	local args={...}
	return function(...)
		local args2 = {...}
		return func(unpack(args), unpack(args2))
	end
end

dofile(folder() .. "global_rt/basic_functions.lua")
