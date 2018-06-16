identifier = "Plugins/Z80"
identifier = string.char(27) .. "[36m" .. identifier .. string.char(27) .. "[00m"

log("Initializing...")
depth(depth() + 1)

log("Preregistering cleanup function in case of error...")
function cleanup()
	log("Unregistering CPU")
	depth(depth() + 1)
	unregisterCPU("Z80")
end

log("Registering plugin message handler...")
messageHandler = function(message)
	return "null"
end

log("Setting up Z80 class...")
Z80 = {}
function Z80.new(o)
	log("Instantiating Z80 CPU object...")
	o = o or {}
	setmetatable(o, {__index = Z80})
	o.registers = {
		af = 0, bc = 0, de = 0, hl = 0,
		af2 = 0, bc2 = 0, de2 = 0, hl2 = 0,
		ix = 0, iy = 0, sp = 0, pc = 0
	}
	o.cycles = 0
	return o
end

Z80.cleanupPlugin = cleanup

log("Registering CPU class...")
registerCPU("Z80", Z80)
-- the above line will error out on failure, so if we reached this point, no error
depth(depth() + 1)
log("Registered successfully!")
depth(depth() - 2)

log("Initialization complete!")
