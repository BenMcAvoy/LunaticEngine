-- NOTE: this code is AI generated, the code all the way further down in the file is not but it is disabled by this code.

-- Tic-tac-toe with a minimax AI (AI plays O, human plays X)
local camera = root:findChildByName("MainCamera")
local mousePos, worldPos
local placingX = true -- true => human (X) turn, false => AI (O) turn

-- Restart delay (seconds)
local RESTART_SECONDS = 5

-- Store board references to sprites
boardSprites = {}

local board = script:getParent()
for i, child in ipairs(board:getChildren()) do
	if child:isA("Sprite") then
		-- board is 3x3, make 2d table
		local x = math.floor((i - 1) / 3) + 1
		local y = (i - 1) % 3 + 1
		boardSprites[x] = boardSprites[x] or {}
		boardSprites[x][y] = {child, 0} -- {sprite, state} ; 0 = empty, 1 = X, 2 = O
	end
end

-- RNG seed once
math.randomseed(os.time() % 2^31)

-- Helpers
local function resetBoard()
	for x = 1, 3 do
		for y = 1, 3 do
			boardSprites[x][y][2] = 0
			boardSprites[x][y][1]:setColor(vec4.new(0, 0, 0, 1))
		end
	end
	placingX = true
end

-- Returns 0, 1, 2. 0 means no win
local function checkWin()
    for i = 1, 3 do
        if boardSprites[i][1][2] == boardSprites[i][2][2] and boardSprites[i][1][2] == boardSprites[i][3][2] and boardSprites[i][1][2] ~= 0 then
            return boardSprites[i][1][2]
        end
        if boardSprites[1][i][2] == boardSprites[2][i][2] and boardSprites[1][i][2] == boardSprites[3][i][2] and boardSprites[1][i][2] ~= 0 then
            return boardSprites[1][i][2]
        end
    end
    if boardSprites[1][1][2] == boardSprites[2][2][2] and boardSprites[1][1][2] == boardSprites[3][3][2] and boardSprites[1][1][2] ~= 0 then
        return boardSprites[1][1][2]
    end
    if boardSprites[3][1][2] == boardSprites[2][2][2] and boardSprites[3][1][2] == boardSprites[1][3][2] and boardSprites[3][1][2] ~= 0 then
        return boardSprites[3][1][2]
    end
    return 0 -- No win
end

-- Helper: returns list of empty cells as { {x,y}, ... }
local function emptyCells()
	local list = {}
	for x = 1, 3 do
		for y = 1, 3 do
			if boardSprites[x][y][2] == 0 then
				table.insert(list, {x, y})
			end
		end
	end
	return list
end

local function isBoardFull()
	return #emptyCells() == 0
end

-- Minimax for tic-tac-toe
-- AI is '2' (O) and tries to maximize score.
-- Human is '1' (X) and tries to minimize score.
-- Scoring: AI win -> +10 - depth, Human win -> -10 + depth, Draw -> 0
local function minimax(isMaximizing, depth)
	local winner = checkWin()
	if winner ~= 0 then
		if winner == 2 then return 10 - depth end
		if winner == 1 then return depth - 10 end
	end

	local empties = emptyCells()
	if #empties == 0 then
		return 0 -- draw
	end

	if isMaximizing then
		local best = -1e9
		for _, cell in ipairs(empties) do
			local x, y = cell[1], cell[2]
			boardSprites[x][y][2] = 2 -- AI move
			local score = minimax(false, depth + 1)
			boardSprites[x][y][2] = 0 -- undo
			if score > best then best = score end
		end
		return best
	else
		local best = 1e9
		for _, cell in ipairs(empties) do
			local x, y = cell[1], cell[2]
			boardSprites[x][y][2] = 1 -- Human move
			local score = minimax(true, depth + 1)
			boardSprites[x][y][2] = 0 -- undo
			if score < best then best = score end
		end
		return best
	end
end

-- Choose best move for AI using minimax, with random tie-breaking among equal-score moves
local function chooseBestMove()
	local bestScore = -1e9
	local bestMoves = {}
	local empties = emptyCells()
	if #empties == 0 then return nil end

	for _, cell in ipairs(empties) do
		local x, y = cell[1], cell[2]
		boardSprites[x][y][2] = 2 -- try
		local score = minimax(false, 0)
		boardSprites[x][y][2] = 0 -- undo

		if score > bestScore then
			bestScore = score
			bestMoves = {{x, y}}
		elseif score == bestScore then
			table.insert(bestMoves, {x, y})
		end
	end

	-- random tie-break
	local pick = bestMoves[math.random(1, #bestMoves)]
	return pick[1], pick[2]
end

-- Fake AI thinking then place best move
local function aiTakeTurn()
	-- If no moves left, just return (draw will be handled in main loop)
	if isBoardFull() then
		return
	end

	-- random think time between 20 and 60 frames
	local thinkFrames = math.random(20, 60)

	for f = 1, thinkFrames do
		local dots = string.rep(".", (f % 3) + 1)
		engine:drawText(vec2.new(0, 0.95), "AI is thinking" .. dots, vec4.new(1, 1, 1, 1))
		yield()
	end

	-- compute best move and play it
	local bx, by = chooseBestMove()
	if bx and by then
		boardSprites[bx][by][2] = 2
		boardSprites[bx][by][1]:setColor(vec4.new(0, 0, 1, 1)) -- immediate visual feedback
	end
end

-- Show result text for RESTART_SECONDS then reset the board
local function showResultThenRestart(win)
	local start = os.clock()
	while (os.clock() - start) < RESTART_SECONDS do
		if win ~= 0 then
			local winText = win == 1 and "X wins!" or "O wins!"
			engine:drawText(vec2.new(0, 0.95), winText, vec4.new(1, 1, 1, 1))
		else
			engine:drawText(vec2.new(0, 0.95), "Draw!", vec4.new(1, 1, 1, 1))
		end
		yield()
	end

	-- After the delay, reset the board
	resetBoard()
end

-- main loop
while true do
    local win = checkWin()
    if win ~= 0 or isBoardFull() then
        -- show result for RESTART_SECONDS then restart
        showResultThenRestart(win)
        -- continue main loop with a fresh board
    end

    -- AI turn - run thinking + move, then hand control back to human
    if not placingX then
        aiTakeTurn()
        placingX = true
        -- allow the frame to process and render the new move
    end

    -- Draw prompt text for the current player
    local text = placingX and "Click to place X" or "Click to place O"
    engine:drawText(vec2.new(0, 0.95), text, vec4.new(1, 1, 1, 1))

    -- Get mouse state (used only if it's the human's turn)
    local leftClick = engine:getMouseButtonState(0)

    -- Process board sprites (hover, placement, coloring)
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

            -- Hover highlight only when human's turn (placingX)
            if inside then
                if boardSprite[2] == 0 and placingX then
                    boardSprite[1]:setColor(vec4.new(0.2, 0.2, 0.2, 1)) -- grey for hover empty
                end

                if placingX and leftClick == 1 then
                    local colour = 1 -- human always X
                    if boardSprite[2] == 0 then -- Only place if the cell is empty
                        boardSprite[2] = colour
                        placingX = not placingX -- Toggle turn (this hands turn to AI if it becomes false)
                    end
                end
            else
                -- reset empty cell color (black background)
                if boardSprite[2] == 0 then
                    boardSprite[1]:setColor(vec4.new(0, 0, 0, 1))
                end
            end

            -- apply state colors (these override hover)
            local state = boardSprite[2]
            if state == 0 then
                -- leave as-is (either hover set above or reset)
            elseif state == 1 then
                boardSprite[1]:setColor(vec4.new(1, 0, 0, 1)) -- Red for X
            elseif state == 2 then
                boardSprite[1]:setColor(vec4.new(0, 0, 1, 1)) -- Blue for O
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
            engine:drawText(vec2.new(0, 0.95), winText, vec4.new(1, 1, 1, 1))
            yield()
        end
    end

    local text = placingX and "Click to place X" or "Click to place O"
    engine:drawText(vec2.new(0, 0.95), text, vec4.new(1, 1, 1, 1))

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