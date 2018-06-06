identifier = "Plugins/Z80"

log("Initializing...")
depth = depth + 1

log("Registering plugin capabilities...")
registerCapability("CPU", "Z80", "z80", "cpu")

log("Registering plugin message handler...")
messageHandler = function(message)
	return "null"
end

log("Setting up Z80 class...")
Z80 = {}
function Z80:new(o)
	o = o or {}
	setmetatable(o, self)
	self.__index = self
	o.registers = {
		af = 0, bc = 0, de = 0, hl = 0,
		af2 = 0, bc2 = 0, de2 = 0, hl2 = 0,
		ix = 0, iy = 0, sp = 0, pc = 0
	}
	o.cycles = 0
	
	return o
end

log("Registering CPU class...")
registerCPU(Z80)
-- the above line will error out on failure, so if we reached this point, no error
depth = depth + 1
log("Registered successfully!")
depth = depth - 2

log("Initialization complete!")
