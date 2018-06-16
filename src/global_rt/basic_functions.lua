local btns_pressed = {
	-- arrows
	false, false, false, false,
	-- abxy
	false, false, false, false
}

local btns_released = {
	-- arrows
	false, false, false, false,
	-- abxy
	false, false, false, false
}

local btns_down = {
	-- arrows
	false, false, false, false,
	-- abxy
	false, false, false, false
}

function processInputs()
	for i = 1, 8 do
		down = btn(i)
		if down then
			btns_pressed[i] = not btns_down[i]
		else
			btns_released[i] = btns_down[i]
			btns_pressed[i] = false
		end
		btns_down[i] = down
	end
end

function btnp(x)
	return btns_pressed[x]
end

function btnr(x)
	return btns_released[x]
end
