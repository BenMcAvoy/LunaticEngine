local camera = root:findChildByName("MainCamera")
local cursor = root:findChildByName("Cursor")

local clickSound = root:findChildByName("ClickSound")
local aiClickSound = root:findChildByName("AIClickSound")

local cursorIndex = 1

local placingX = true
local twoPlayer = false

local cells = {{},{},{}} -- table of rows (3x3)

local iterations = 0

local function initBoard()
	local idx = 0
	for _, child in ipairs(script.parent.children) do
		if child:isA("Sprite") then
			idx = idx + 1
			local row = math.floor((idx - 1) / 3) + 1
			local col = ((idx - 1) % 3) + 1
			cells[row][col] = child
		end
	end

	for i, col in pairs(cells) do
		for j, cell in pairs(col) do
			-- Clear our old textures
			cell.data["state"] = "default"
			cell:clearTexture()
		end
	end
end
initBoard()

-- @param a Sprite
-- @param b vec2
-- @ret bool
local function checkCollision(a, b)
	local aPos = a.position
	local aHalf = 0.5 * a.scale

	return (b.x >= aPos.x - aHalf.x and b.x <= aPos.x + aHalf.x) and
	       (b.y >= aPos.y - aHalf.y and b.y <= aPos.y + aHalf.y)
end

-- @param string (the texture name, e.g. `x` or `o`)
-- @ret string (random texture path, 1-3)
local function getRandomTexPath(name)
	local suffix = math.random(1, 3)
	return "res/textures/"..name..suffix..".png"
end

-- Handle the user selecting a cell (either player)
local function handleUserAction()
	local mousePos = engine:getMousePos()
	local mouseWPos = camera:screenToWorld(mousePos)

	for i, col in pairs(cells) do
		for j, cell in pairs(col) do
			-- If the mouse is over this cell
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
					cell.color = vec4.new(1, 1, 1, 1)
					placingX = not placingX
					clickSound:play()
				end
			else
				-- Not hovered, reset to default if possible (make sure we don't override X or O state)
				if cell.data["state"] == "hovered" then
					cell.data["state"] = "default"
				end
			end
		end
	end
end

-- @ret string "x" | "o" | "draw" | "none"
local function checkWinner()
	-- Check rows
	for i = 1, 3 do
		if cells[i][1].data["state"] == cells[i][2].data["state"] and
		   cells[i][2].data["state"] == cells[i][3].data["state"] then
			local state = cells[i][1].data["state"]
			if state == "x" or state == "o" then
				return state
			end
		end
	end

	-- Check columns
	for i = 1, 3 do
		if cells[1][i].data["state"] == cells[2][i].data["state"] and
		   cells[2][i].data["state"] == cells[3][i].data["state"] then
			local state = cells[1][i].data["state"]
			if state == "x" or state == "o" then
				return state
			end
		end
	end

	-- Check diagonals
	if cells[1][1].data["state"] == cells[2][2].data["state"] and
	   cells[2][2].data["state"] == cells[3][3].data["state"] then
		local state = cells[1][1].data["state"]
		if state == "x" or state == "o" then
			return state
		end
	end
	if cells[1][3].data["state"] == cells[2][2].data["state"] and
	   cells[2][2].data["state"] == cells[3][1].data["state"] then
		local state = cells[1][3].data["state"]
		if state == "x" or state == "o" then
			return state
		end
	end

	-- Check for draw (we check every cell is not default)
	local isDraw = true
	for i = 1, 3 do
		for j = 1, 3 do
			if cells[i][j].data["state"] == "default" then
				isDraw = false
			end
		end
	end
	if isDraw then
		return "draw"
	end

	return "none"
end

local function minimax(maximizing, depth)
	local w = checkWinner()
	if w == "o" then return 10 - depth end
	if w == "x" then return depth - 10 end
	if w == "draw" then return 0 end

	local best = maximizing and -math.huge or math.huge
	for i=1,3 do for j=1,3 do
		if cells[i][j].data.state == "default" then
			cells[i][j].data.state = maximizing and "o" or "x"
			local score = minimax(not maximizing, depth+1)

			cells[i][j].data.state = "default"
			if maximizing then
				best = math.max(best, score)
			else
				best = math.min(best, score)
			end
		end
	end end
	return best
end

-- Count how many cells are filled to determine if this is the first AI move
local function countFilledCells()
    local count = 0
    for i=1,3 do
        for j=1,3 do
            if cells[i][j].data.state ~= "default" then
                count = count + 1
            end
        end
    end
    return count
end

-- Basic heuristic for the first AI move
local function makeFirstAIMove()
    if cells[2][2].data.state == "default" then
        return 2, 2
    end
    
    -- If player took center, take a corner
    if cells[1][1].data.state == "default" then
        return 1, 1
    elseif cells[1][3].data.state == "default" then
        return 1, 3
    elseif cells[3][1].data.state == "default" then
        return 3, 1
    elseif cells[3][3].data.state == "default" then
        return 3, 3
    end
    
    -- If all corners are taken, take any available edge
    if cells[1][2].data.state == "default" then
        return 1, 2
    elseif cells[2][1].data.state == "default" then
        return 2, 1
    elseif cells[2][3].data.state == "default" then
        return 2, 3
    elseif cells[3][2].data.state == "default" then
        return 3, 2
    end
    
    return nil, nil -- No moves available
end

function handleAITurn()
    local bi, bj -- best move coordinates (x, y)

    -- If this is the first AI move (only one X on the board)
    if countFilledCells() == 1 then
        bi, bj = makeFirstAIMove()
    else
        -- Use minimax for subsequent moves
        local bestScore = -math.huge
        for i=1,3 do for j=1,3 do
            if cells[i][j].data.state == "default" then
                -- Try the move, score it, then revert back to "default" so draw checks remain correct
                cells[i][j].data.state = "o"
                local score = minimax(false, 0)
                cells[i][j].data.state = "default"
                if score > bestScore then
                    bestScore, bi, bj = score, i, j
                end
            end
        end end
    end
    
    if bi then
        print(tostring(bi)..","..tostring(bj))
        local cell = cells[bi][bj]
        cell.data.state = "o"
        cell.texturePath = getRandomTexPath("o")
        cell.color = vec4.new(1, 1, 1, 1)
        placingX = true
        aiClickSound:play()
    end
end

-- Main loop
while true do
	local mousePos = engine:getMousePos()
	local worldPos = camera:screenToWorld(mousePos)

	cursor.position = worldPos
	engine:hideCursor()

	iterations = iterations + 1
	if iterations >= 20 then
		cursorIndex = (cursorIndex % 3) + 1
		iterations = 0

		if twoPlayer then
			local textureName = placingX and "x" or "o"
			cursor.texturePath = getRandomTexPath(textureName)
		else
			cursor.texturePath = getRandomTexPath("x")
		end
	end

	-- Update cell visuals once per frame
	for i=1,3 do
		for j=1,3 do
			local cell = cells[i][j]
			local cellState = cell.data["state"]
			if cellState == "default" then
				cell.color = vec4.new(0.2, 0.2, 0.2, 0.2)
			elseif cellState == "hovered" then
				cell.color = vec4.new(0.2, 0.2, 0.2, 0.3)
			elseif cellState == "x" or cellState == "o" then
				cell.color = vec4.new(1, 1, 1, 1)
			end
		end
	end

	-- Winner check once per frame
	local winner = checkWinner()
	if winner ~= "none" then
		local iters = 0
		while iters < 120 do
			iters = iters + 1
			engine:drawText(vec2.new(0, 0.95), winner == "draw" and "It's a draw!" or (winner.." wins!"), vec4.new(0, 0, 0, 1))
			yield()
		end
		initBoard()
	end

	-- Handle a single action per frame: user or AI
	if twoPlayer or placingX then
		handleUserAction()
	else
		handleAITurn()
	end
	
	yield()
end