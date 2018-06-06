identifier = "Plugins/SimpleShell"

log("Initializing...")
depth = depth + 1

log("Setting up commands table...")
commands = {
	echo = {
		func = function(x)
			history:add(x)
		end
	}
}

log("Setting up command history...")
history = {
	-- Stores the actual commands. This is used for command repeating.
	commands = {},
	-- Stores all output. This is what is rendered.
	lines = {}
}

function history:add(message, command)
	if type(command) ~= "boolean" then command = false end
	
end

function history:clear()
	self.commands = {}
	self.lines = {}
end

function history:getCommand(x)
	
end

depth = depth - 1
