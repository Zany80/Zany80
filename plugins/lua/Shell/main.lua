identifier = "Plugins/SimpleShell"
identifier = string.char(27) .. "[36m" .. identifier .. string.char(27) .. "[00m"

local GLYPHS_PER_LINE = (LCD_WIDTH / (GLYPH_WIDTH))

local plugin_version = "0.1.0"

log("Initializing...")
depth(depth() + 1)

local command = ""
-- How many lines are needed for the command?
local command_lines = 1
local old_command = {
	current_command = "",
	index = 0
}

local red = {g = 0, b = 0}
local green = {r = 0, b = 0}

local function getValue(v)
	if type(v) == "boolean" then return v and "true" or "false" end
	if type(v) == "number" then return "" .. v end
	if type(v) == "string" then return "\""..v.."\"" end
	return type(v)
end

log("Setting up commands table...")
commands = {
	echo = {
		execute = function(x)
			history:add(table.concat(x, " "), false)
		end
	},
	help = {
		execute = function(x)
			if #x == 0 then
				history:add("")
				for k, v in pairs(commands) do
					if v.help ~= nil then
						history:add(k .. ": "..v.help, false, green)
					else
						history:add(k .. ": no help is available for this command. Sorry!", false, red)
					end
				end
			else
				for i, v in ipairs(x) do
					if commands[v] ~= nil then
						if commands[v].detailed_help ~= nil then
							history:add(commands[v].detailed_help)
						else
							history:add("Detailed help is unavailable for command `"..v.."`")
						end
					else
						history:add("No such command: `"..v.."`")
					end
				end
			end
		end,
		help = "Prints out information on commands. `help help` gives more information.",
		detailed_help = "Test\nt"
	},
	exec = {
		execute = function(x)
			local cx = table.concat:bind(x, " ")
			results = {pcall(loadstring("return "..cx()) or loadstring(cx()) or error:bind("Invalid Lua command!", 0))}
			if results[1] then
				table.remove(results, 1)
				for k, v in ipairs(results) do
					results[k] = getValue(v)
				end
				msg = table.concat(results, ", ")
				history:add(#msg > 0 and msg or "nil", false)
			else
				history:add("Error: "..results[2], false)
			end
		end
	}
}

log("Setting up command history...")
history = {
	-- Stores the actual commands. This is used for command repeating.
	commands = {},
	-- Stores all output. This is what is rendered.
	lines = {},
	scroll = 0
}

function history:add(message, command, color)
	if type(command) ~= "boolean" then command = false end
	while #message > GLYPHS_PER_LINE do
		self.lines[#self.lines + 1] = {
			message = message:sub(1, GLYPHS_PER_LINE),
			color = color or {r = 255, g = 255, b = 255}
		}
		message = message:sub(GLYPHS_PER_LINE + 1)
	end
	self.lines[#self.lines + 1] = {
		message = message,
		color = color or {r = 255, g = 255, b = 255}
	}
	if command then
		self.commands[#self.commands + 1] = message
	end
end

function history:getColor(i)
	local i = #self.lines - i + 1
	return self.lines[i].color.r or 255,self.lines[i].color.g or 255,self.lines[i].color.b or 255
end

function history:getLine(i)
	local i = #self.lines - i + 1
	return self.lines[i].message
end

function history:clear()
	self.commands = {}
	self.lines = {}
	self.scroll = 0
end

function history:getCommand(i)
	local i = #self.commands - i + 1
	return self.commands[i]
end

depth(depth() - 1)

function cleanup()
	log("Cleaning up...")
	depth(depth() + 1)
	history:save()
	depth(depth() - 1)
end

function executeCommand(command)
	words = {}
	for w in command:gmatch("%S+") do words[#words + 1] = w end
	local command = words[1]
	table.remove(words, 1)
	if commands[command] ~= nil then
		commands[command].execute(words)
	else
		history:add("Command `"..command.."` doesn't exist!", false)
	end
end

function processKey(key)
	--history:add("Key: {special = "..(key.special and "true" or "false")..", code = "..(key.code or "nil")..", character = "..(key.character or "nil"))
	if key.special then
		if key.code == "enter" then
			if #command == 0 then return end
			history:add(command, true)
			executeCommand(command)
			command = ""
			command_lines = 1
			old_command.current_command = ""
			old_command.index = 0
		end
		if key.code == "backspace" then
			command = command:sub(1, -2)
		end
		if key.code == "up" then
			if old_command.index == 0 then old_command.current_command = command end
			if history:getCommand(old_command.index + 1) ~= nil then
				old_command.index = old_command.index + 1
				command = history:getCommand(old_command.index)
			end
		end
		if key.code == "down" then
			if old_command.index > 0 then
				old_command.index = old_command.index - 1
				if old_command.index == 0 then
					command = old_command.current_command
				else
					command = history:getCommand(old_command.index)
				end
			end
		end
	else
		command = command .. string.char(key.character)
		command_lines = 1 + math.floor(#command / GLYPHS_PER_LINE)
	end
	command_lines = 1 + math.floor(#command / GLYPHS_PER_LINE)
end

local function render()
	local y = LCD_HEIGHT - (command_lines + 1) * GLYPH_HEIGHT
	for i = 1, command_lines do
		y = y + GLYPH_HEIGHT
		text(string.sub(command, (i - 1) * GLYPHS_PER_LINE, i * GLYPHS_PER_LINE), 0, y)
	end
	for i = 1, #history.lines do
		y = y - GLYPH_HEIGHT
		if y - history.scroll <= (LCD_HEIGHT - GLYPH_HEIGHT * (command_lines + 1)) then
			text(history:getLine(i), 0, y - history.scroll, history:getColor(i))
		end
		if y - history.scroll < 0 then break end
	end
end

local function registerPlugin(plugin)
	log("Registering plugin " .. plugin.name)
end

registerExecutable("SimpleShell", {
	frame = render,
	key = processKey,
	registerPlugin = registerPlugin
})

history:add("SimpleShell v"..plugin_version, false, {r = 0, g = 160, b = 180})
history:add("Welcome to the Simple Shell.", false)
history:add("", false)
history:add("Type `help` to get started!", false)
