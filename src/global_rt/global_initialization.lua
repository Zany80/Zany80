cpus={}

depth = 1

function cpus:enumerate()
	for k, v in pairs(self) do
		if type(v) == "table" and not v.enumerated then
			self[#self + 1] = v v.enumerated = true
		end
	end
end
