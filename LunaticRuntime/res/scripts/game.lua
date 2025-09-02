local camera = root:findChildByName("MainCamera")
local placingX = true
local twoPlayer = true

local cells = {{},{}, {}} -- table of rows

for i, child in pairs(script.parent.children) do
	row = 0
	col = 0

	col = col + 1
	if col >= 3 then
		row = row + 1
		col = 0
	end

	if child:isA("Sprite") then
		table.insert(cells[row * 3 + col + 1], child)
	end
end

for i, col in pairs(cells) do
	for j, cell in pairs(col) do
		cell.data["state"] = "default"
		cell:clearTexture()
	end
end

-- @param a Sprite
-- @param b vec2
-- @ret bool
local function checkCollision(a, b)
	local aPos = a.position
	local aHalf = 0.5 * a.scale

	return (b.x >= aPos.x - aHalf.x and b.x <= aPos.x + aHalf.x) and
	       (b.y >= aPos.y - aHalf.y and b.y <= aPos.y + aHalf.y)
end

-- @param string (the texture name)
-- @ret string (random texture path, 1-3)
local function getRandomTexPath(name)
	local suffix = math.random(1, 3)
	return "res/textures/"..name..suffix..".png"
end

local function handleUserAction()
	local mousePos = engine:getMousePos()
	local mouseWPos = camera:screenToWorld(mousePos)

	for i, col in pairs(cells) do
		for j, cell in pairs(col) do
			if checkCollision(cell, mouseWPos) then
				-- Hovered
				if cell.data["state"] == "default" then
					cell.data["state"] = "hovered"
				end

				-- Check click
				local left = engine:getMouseButtonState(0)
				if left == 1 then
					cell.data["state"] = placingX and "x" or "o"
					cell.texturePath = placingX and getRandomTexPath("x") or getRandomTexPath("o")
					placingX = not placingX
				end
			else
				-- Not hovered (make sure we don't override X or O state)
				if cell.data["state"] == "hovered" then
					cell.data["state"] = "default"
				end
			end
		end
	end
end

-- Main loop
while true do
	for i, col in pairs(cells) do
		for j, cell in pairs(col) do
			if twoPlayer or placingX then
				handleUserAction()
			else
				-- TODO: AI turn
			end

			-- Update visuals
			local cellState = cell.data["state"]
			if cellState == "default" then
				cell.color = vec4.new(0.2, 0.2, 0.2, 0.1)
			elseif cellState == "hovered" then
				cell.color = vec4.new(0.2, 0.2, 0.2, 0.3)
			elseif cellState == "x" or cellState == "o" then
				cell.color = vec4.new(1, 1, 1, 1)
			end
		end
	end
	
	yield()
end