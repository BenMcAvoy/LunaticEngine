-- NOTE: this code is AI generated, the code all the way further down in the file is not but it is disabled by this code.

-- Tic-tac-toe with a minimax AI (AI = O, human = X)
-- Polished: proper texture/color handling, reset, hover, restart delay, and win highlight.

local camera = root:findChildByName("MainCamera")
local soundEmitter = script.parent:findChildByName("SoundEmitter")

local placingX = true -- true = human (X), false = AI (O)

-- Restart delay in seconds
local RESTART_SECONDS = 5

-- Board storage: boardSprites[x][y] = { sprite, state }
-- state: 0 = empty, 1 = X, 2 = O
local boardSprites = {}

-- Build board references (assumes 3x3 sprites are children in deterministic order)
local board = script.parent
for i, child in ipairs(board.children) do
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
	soundEmitter.soundPath = "res/sounds/click.wav"
	soundEmitter:play()
	boardSprites[x][y][2] = 1
	boardSprites[x][y][1].texturePath = randomVariantTexture("res/textures/cursors/cursor")
	boardSprites[x][y][1].color = vec4.new(1, 1, 1, 1)
end

local function placeOAt(x, y)
	soundEmitter.soundPath = "res/sounds/clickBack.wav"
	soundEmitter:play()
	boardSprites[x][y][2] = 2
	boardSprites[x][y][1].texturePath = randomVariantTexture("res/textures/circles/circle")
	boardSprites[x][y][1].color = vec4.new(1, 1, 1, 1)
end

-- Reset board to very transparent black tiles
local function resetBoard()
	for x = 1, 3 do
		for y = 1, 3 do
			boardSprites[x][y][2] = 0
			boardSprites[x][y][1]:clearTexture()
			boardSprites[x][y][1].color = vec4.new(0, 0, 0, 0.08)
		end
	end
	placingX = true
end

-- Win checker: returns 0 (none), 1 (X), 2 (O)
local function checkWin()
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
		if winner == 2 then return 10 - depth end
		if winner == 1 then return depth - 10 end
	end

	local empties = emptyCells()
	if #empties == 0 then return 0 end

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
					sprite.color = vec4.new(1, 1, 1, 1)
				else
					sprite.color = vec4.new(0.25, 0.25, 0.25, 1)
				end
			end
		end
	end
end

-- Show result text then restart
local function showResultThenRestart(win, winCells)
	if win ~= 0 and winCells then
		applyWinHighlight(winCells)
	end

	if win == 0 then
		soundEmitter.soundPath = "res/sounds/powerUp.wav"
	else
		soundEmitter.soundPath = "res/sounds/explosion.wav"
	end

	soundEmitter:play()

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
	local win, winCells = checkWinDetailed()
	if win ~= 0 or isBoardFull() then
		showResultThenRestart(win, winCells)
	end

	if not placingX then
		aiTakeTurn()
		placingX = true
	end

	local prompt = placingX and "Click to place X" or "AI thinking..."
	engine:drawText(vec2.new(0, 0.95), prompt, vec4.new(0.2, 0.2, 0.2, 1))

	local leftClick = engine:getMouseButtonState(0)
	local mousePos = engine:getMousePos()
	local worldPos = camera:screenToWorld(mousePos)

	for x = 1, 3 do
		for y = 1, 3 do
			local boardSprite = boardSprites[x][y]
			local sprite = boardSprite[1]
			local state = boardSprite[2]

			local pos = sprite.position
			local half = 0.25

			local inside =
				worldPos.x >= pos.x - half and worldPos.x <= pos.x + half and
				worldPos.y >= pos.y - half and worldPos.y <= pos.y + half

			if inside and state == 0 and placingX then
				sprite.color = vec4.new(0.15, 0.15, 0.15, 0.18)
			elseif state == 0 then
				sprite:clearTexture()
				sprite.color = vec4.new(0, 0, 0, 0.08)
			end

			if placingX and inside and leftClick == 1 and state == 0 then
				placeXAt(x, y)
				placingX = false
			end
		end
	end

	yield()
end