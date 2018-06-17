validatePlugins()
cpus:enumerate()
executables:enumerate()

if #executables == 0 then
	error("No executables detected!")
end

if #cpus == 0 then
	error("No CPUs found!")
end

executable = nil
selected_executable = 1

function chooseExecutable()
	background(0, 0, 0)
	text("Select executable: (asterisk indicates selected executable)", 0, 0)
	local y = 4
	for k, v in ipairs(executables) do
		y = y + 10
		local message = k .. ". "..v.enumerated
		if selected_executable == k then message = message .. " *" end
		text(message, 0, y)
	end
	if btnp(1) then selected_executable = selected_executable - 1 end
	if btnp(2) then selected_executable = selected_executable + 1 end
	if selected_executable < 1 then selected_executable = 1 end
	if selected_executable > #executables then selected_executable = #executables end
	if btnp(5) then executable = executables[selected_executable] clearEventQueue() end
end

function loop()
	processInputs()
	if executable == nil then
		chooseExecutable()
	else
		executable.frame()
	end
end

function processEvent(event)
	if (event.type == "TextEntered" and event.character >= 32 and event.character <= 127) or event.type == "KeyPressed" then
		event.special = event.type=="KeyPressed"
		if type(executable.key) == "function" then
			success, message = pcall(executable.key:bind(event))
			if not success then print(message) end
		end
	end
end
