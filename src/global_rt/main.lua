validatePlugins()
cpus:enumerate()

for k, v in pairs(cpus) do
	if type(v) == "table" and tonumber(k) == nil then
		print("Found CPU: "..k)
	end
end

if #cpus == 0 then
	error("No CPUs found!")
end

z80 = cpus.Z80.new()
print(type(z80.new))

background(3, 244, 187)
