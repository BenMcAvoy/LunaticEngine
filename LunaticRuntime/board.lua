-- NOTE: this code is AI generated, the code all the way further down in the file is not but it is disabled by this code.

-- Tic-tac-toe with a minimax AI (AI = O, human = X)
-- Polished: proper texture/color handling, reset, hover, restart delay, and win highlight.

local camera = root:findChildByName("MainCamera")
local placingX = true -- true = human (X), false = AI (O)

-- Restart delay in seconds
local RESTART_SECONDS = 5

-- Board storage: boardSprites[x][y] = { sprite, state }
-- state: 0 = empty, 1 = X, 2 = O
local boardSprites = {}

-- Build board references (assumes 3x3 sprites are children in deterministic order)
local board = script:getParent()
for i, child in ipairs(board:getChildren()) do
	if child:isA("Sprite") then
		local x = math.floor((i - 1) / 3) + 1
		local y = (i - 1) % 3 + 1
		boardSprites[x] = boardSprites[x] or {}
		boardSprites[x][y] = { child, 0 }
	end
end

-- RNG seed once
math.randomseed(os.time() % 2^31)

-- Helper to pick a random variant 1..3 for a base name (e.g., "circle" -> "circle2.png")
local function randomVariantTexture(base)
	return string.format("%s%d.png", base, math.random(1, 3))
end

-- Helpers to set X and O (ensures texture + white color)
local function placeXAt(x, y)
	boardSprites[x][y][2] = 1
	boardSprites[x][y][1]:setTexture(randomVariantTexture("cursor"))
	-- White = max brightness for textured sprites with this shader
	boardSprites[x][y][1]:setColor(vec4.new(1, 1, 1, 1))
end
local function placeOAt(x, y)
	boardSprites[x][y][2] = 2
	boardSprites[x][y][1]:setTexture(randomVariantTexture("circle"))
	-- White = max brightness for textured sprites with this shader
	boardSprites[x][y][1]:setColor(vec4.new(1, 1, 1, 1))
end

-- Reset board to very transparent black tiles
local function resetBoard()
	for x = 1, 3 do
		for y = 1, 3 do
			boardSprites[x][y][2] = 0
			boardSprites[x][y][1]:clearTexture()
			-- Set to very transparent black (suggests interaction, not a sprite)
			boardSprites[x][y][1]:setColor(vec4.new(0, 0, 0, 0.08))
		end
	end
	placingX = true
end

-- Win checker: returns 0 (none), 1 (X), 2 (O)
local function checkWin()
	-- Keep this for minimax
	for i = 1, 3 do
		-- rows
		if boardSprites[i][1][2] ~= 0 and boardSprites[i][1][2] == boardSprites[i][2][2] and boardSprites[i][1][2] == boardSprites[i][3][2] then
			return boardSprites[i][1][2]
		end
		-- cols
		if boardSprites[1][i][2] ~= 0 and boardSprites[1][i][2] == boardSprites[2][i][2] and boardSprites[1][i][2] == boardSprites[3][i][2] then
			return boardSprites[1][i][2]
		end
	end
	-- diagonals
	if boardSprites[1][1][2] ~= 0 and boardSprites[1][1][2] == boardSprites[2][2][2] and boardSprites[1][1][2] == boardSprites[3][3][2] then
		return boardSprites[1][1][2]
	end
	if boardSprites[3][1][2] ~= 0 and boardSprites[3][1][2] == boardSprites[2][2][2] and boardSprites[3][1][2] == boardSprites[1][3][2] then
		return boardSprites[3][1][2]
	end
	return 0
end

-- Detailed win checker: returns winner and the 3 winning cells as {{x,y}, {x,y}, {x,y}}
local function checkWinDetailed()
	for i = 1, 3 do
		-- rows
		if boardSprites[i][1][2] ~= 0 and boardSprites[i][1][2] == boardSprites[i][2][2] and boardSprites[i][1][2] == boardSprites[i][3][2] then
			return boardSprites[i][1][2], { { i, 1 }, { i, 2 }, { i, 3 } }
		end
		-- cols
		if boardSprites[1][i][2] ~= 0 and boardSprites[1][i][2] == boardSprites[2][i][2] and boardSprites[1][i][2] == boardSprites[3][i][2] then
			return boardSprites[1][i][2], { { 1, i }, { 2, i }, { 3, i } }
		end
	end
	-- diagonals
	if boardSprites[1][1][2] ~= 0 and boardSprites[1][1][2] == boardSprites[2][2][2] and boardSprites[1][1][2] == boardSprites[3][3][2] then
		return boardSprites[1][1][2], { { 1, 1 }, { 2, 2 }, { 3, 3 } }
	end
	if boardSprites[3][1][2] ~= 0 and boardSprites[3][1][2] == boardSprites[2][2][2] and boardSprites[3][1][2] == boardSprites[1][3][2] then
		return boardSprites[3][1][2], { { 3, 1 }, { 2, 2 }, { 1, 3 } }
	end
	return 0, nil
end

local function emptyCells()
	local list = {}
	for x = 1, 3 do
		for y = 1, 3 do
			if boardSprites[x][y][2] == 0 then
				table.insert(list, { x, y })
			end
		end
	end
	return list
end

local function isBoardFull()
	return #emptyCells() == 0
end

-- Minimax (AI tries to maximize; human tries to minimize)
local function minimax(isMaximizing, depth)
	local winner = checkWin()
	if winner ~= 0 then
		if winner == 2 then return 10 - depth end -- AI win
		if winner == 1 then return depth - 10 end -- Human win
	end

	local empties = emptyCells()
	if #empties == 0 then return 0 end -- draw

	if isMaximizing then
		local best = -1e9
		for _, cell in ipairs(empties) do
			local x, y = cell[1], cell[2]
			boardSprites[x][y][2] = 2
			local score = minimax(false, depth + 1)
			boardSprites[x][y][2] = 0
			if score > best then best = score end
		end
		return best
	else
		local best = 1e9
		for _, cell in ipairs(empties) do
			local x, y = cell[1], cell[2]
			boardSprites[x][y][2] = 1
			local score = minimax(true, depth + 1)
			boardSprites[x][y][2] = 0
			if score < best then best = score end
		end
		return best
	end
end

-- Choose best move with random tie-breaking
local function chooseBestMove()
	local bestScore = -1e9
	local bestMoves = {}
	local empties = emptyCells()
	if #empties == 0 then return nil end

	for _, cell in ipairs(empties) do
		local x, y = cell[1], cell[2]
		boardSprites[x][y][2] = 2
		local score = minimax(false, 0)
		boardSprites[x][y][2] = 0

		if score > bestScore then
			bestScore = score
			bestMoves = { { x, y } }
		elseif score == bestScore then
			table.insert(bestMoves, { x, y })
		end
	end

	local pick = bestMoves[math.random(1, #bestMoves)]
	return pick[1], pick[2]
end

-- AI "thinking" then place move
local function aiTakeTurn()
	if isBoardFull() then return end

	local thinkFrames = math.random(20, 60)
	for f = 1, thinkFrames do
		local dots = string.rep(".", (f % 3) + 1)
		engine:drawText(vec2.new(0, 0.95), "AI is thinking" .. dots, vec4.new(0.2, 0.2, 0.2, 1))
		yield()
	end

	local bx, by = chooseBestMove()
	if bx and by then
		placeOAt(bx, by)
	end
end

-- Apply win highlight: keep winning cells bright (white), dim other placed cells
local function applyWinHighlight(winCells)
	if not winCells then return end
	local set = {}
	for _, c in ipairs(winCells) do
		set[c[1] .. "," .. c[2]] = true
	end
	for x = 1, 3 do
		for y = 1, 3 do
			local sprite = boardSprites[x][y][1]
			local state = boardSprites[x][y][2]
			if state ~= 0 then
				if set[x .. "," .. y] then
					-- Full brightness for winners
					sprite:setColor(vec4.new(1, 1, 1, 1))
				else
					-- Dim non-winning marks to emphasize the line
					sprite:setColor(vec4.new(0.25, 0.25, 0.25, 1))
				end
			else
				-- leave empty cells as-is (already very faint)
			end
		end
	end
end

-- Show result text then restart
local function showResultThenRestart(win, winCells)
	-- Highlight winning connection if any
	if win ~= 0 and winCells then
		applyWinHighlight(winCells)
	end

	local start = os.clock()
	while (os.clock() - start) < RESTART_SECONDS do
		if win ~= 0 then
			local winText = win == 1 and "X wins!" or "O wins!"
			engine:drawText(vec2.new(0, 0.95), winText, vec4.new(0.2, 0.2, 0.2, 1))
		else
			engine:drawText(vec2.new(0, 0.95), "Draw!", vec4.new(0.2, 0.2, 0.2, 1))
		end
		yield()
	end
	resetBoard()
end

-- Main loop
while true do
	-- If finished (win or full) show result then restart
	local win, winCells = checkWinDetailed()
	if win ~= 0 or isBoardFull() then
		showResultThenRestart(win, winCells)
		-- after this returns, board is reset
	end

	-- AI turn
	if not placingX then
		aiTakeTurn()
		placingX = true
	end

	-- Draw prompt for player
	local prompt = placingX and "Click to place X" or "AI thinking..."
	engine:drawText(vec2.new(0, 0.95), prompt, vec4.new(0.2, 0.2, 0.2, 1))

	-- Get input once per frame
	local leftClick = engine:getMouseButtonState(0)
	local mousePos = engine:getMousePos()
	local worldPos = camera:screenToWorld(mousePos)

	-- Process cells explicitly in deterministic order (1..3)
	for x = 1, 3 do
		for y = 1, 3 do
			local boardSprite = boardSprites[x][y]
			local sprite = boardSprite[1]
			local state = boardSprite[2]

			local pos = sprite:getPosition()
			local half = 0.25 -- sprite is 0.5x0.5

			local inside =
				worldPos.x >= pos.x - half and worldPos.x <= pos.x + half and
				worldPos.y >= pos.y - half and worldPos.y <= pos.y + half

			-- Hover highlight only for empty cells and only on player's turn
			if inside and state == 0 and placingX then
				-- Slightly more visible on hover (suggest interaction, not presence)
				sprite:setColor(vec4.new(0.15, 0.15, 0.15, 0.18))
			elseif state == 0 then
				-- ensure empty non-hover cells are very transparent black, no texture
				sprite:clearTexture()
				sprite:setColor(vec4.new(0, 0, 0, 0.08))
			end

			-- Human placement: on left mouse held down (no debounce)
			if placingX and inside and leftClick == 1 and state == 0 then
				placeXAt(x, y)
				placingX = false -- hand turn to AI
			end
		end
	end

	yield()
end









































-- Below is very sketchy, non AI generated code that I was not thinking about
-- It is terrible in quality, thrown together, and not given a second thought
-- Please expect nothing from it

local camera = root:findChildByName("MainCamera")
local mousePos, worldPos
local placingX = true

-- black = none
-- red = x
-- blue = o

-- Store board references to sprites
boardSprites = {}

local board = script:getParent()
for i, child in ipairs(board:getChildren()) do
	if child:isA("Sprite") then
		-- board is 3x3, make 2d table
		local x = math.floor((i - 1) / 3) + 1
		local y = (i - 1) % 3 + 1
		boardSprites[x] = boardSprites[x] or {}
		boardSprites[x][y] = {child, 0} -- child, 0 is the default state
	end
end

-- Returns 0, 1, 2. 0 means no win
function checkWin()
    -- Check rows and columns
    for i = 1, 3 do
        if boardSprites[i][1][2] == boardSprites[i][2][2] and boardSprites[i][1][2] == boardSprites[i][3][2] and boardSprites[i][1][2] ~= 0 then
            return boardSprites[i][1][2] -- Return the winning state (1 or 2)
        end
        if boardSprites[1][i][2] == boardSprites[2][i][2] and boardSprites[1][i][2] == boardSprites[3][i][2] and boardSprites[1][i][2] ~= 0 then
            return boardSprites[1][i][2] -- Return the winning state (1 or 2)
        end
    end
    -- Check diagonals
    if boardSprites[1][1][2] == boardSprites[2][2][2] and boardSprites[1][1][2] == boardSprites[3][3][2] and boardSprites[1][1][2] ~= 0 then
        return boardSprites[1][1][2]
    end
    if boardSprites[3][1][2] == boardSprites[2][2][2] and boardSprites[3][1][2] == boardSprites[1][3][2] and boardSprites[3][1][2] ~= 0 then
        return boardSprites[3][1][2]
    end
    return 0 -- No win
end

while true do
    local win = checkWin()
    if win ~= 0 then
        while true do
            local winText = win == 1 and "X wins!" or "O wins!"
            engine:drawText(vec2.new(0, 0.95), winText, vec4.new(0.2, 0.2, 0.2, 1))
            yield()
        end
    end

    local text = placingX and "Click to place X" or "Click to place O"
    engine:drawText(vec2.new(0, 0.95), text, vec4.new(0.2, 0.2, 0.2, 1))

    local leftClick = engine:getMouseButtonState(0)

    for x, v in pairs(boardSprites) do
        for y, boardSprite in pairs(v) do
            local position = boardSprite[1]:getPosition()
            local half = 0.25 -- sprite is 0.5x0.5, so half-size = 0.25

            mousePos = engine:getMousePos()
            worldPos = camera:screenToWorld(mousePos)

            -- 2D AABB check (ignore Z)
            local inside =
                worldPos.x >= position.x - half and worldPos.x <= position.x + half and
                worldPos.y >= position.y - half and worldPos.y <= position.y + half

            if inside then
                if boardSprite[2] == 0 then
                    boardSprite[1]:setColor(vec4.new(0.2, 0.2, 0.2, 1)) -- White for empty cell
                end

                if leftClick == 1 then
                    colour = placingX and 1 or 2 -- 1 for X, 2 for O
                    if boardSprite[2] == 0 then -- Only place if the cell is empty
                        boardSprite[2] = colour
                        placingX = not placingX -- Toggle between X and O
                    end
                end
            else
                boardSprite[1]:setColor(vec4.new(0, 0, 0, 1)) -- White for empty cell
            end

            local state = boardSprite[2]
            if state == 0 then
                -- No state, do nothing
            elseif state == 1 then
                boardSprite[1]:setColor(vec4.new(1, 0, 0, 1)) -- Red for X
            elseif state == 2 then
                boardSprite[1]:setColor(vec4.new(0, 0, 1, 1)) -- Blue for O
            end
        end
    end

    yield()
end