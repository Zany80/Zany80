plugins = {
	{
		name = "Zilog Z80 Core",
		path = "$ZANY/Z80",
		description = "Emulation core for the Zilog Z80."
	},
	{
		name = "Simple Shell",
		path = "$ZANY/Shell",
		description = "The title is pretty self explanatory. Enter commands and such."
	}
}

-- Set to 1 to use the main loop from $ZANY/global_rt/main.lua
-- 0 to use a C-based loop
main_loop_mode = 1
