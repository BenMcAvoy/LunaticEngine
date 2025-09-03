local pipesHolder = script.parent
pipesHolder.data["pipes"] = pipesHolder.data["pipes"] or {}

-- Destroy existing pipes
for _, pair in pairs(pipesHolder.data["pipes"]) do
    pair.top:destroy()
    pair.bottom:destroy()
end
pipesHolder.data["pipes"] = {}

local gapSize = 1.5
local minGapY = -1.1
local maxGapY = 1.6

for i = 1, 5 do
    local topPipe = Instance.new("PhysicsSprite", "pipeTop"..i)
    topPipe.bodyType = "static"
    topPipe.scale = vec2.new(0.5, 2.5)
    topPipe.parent = pipesHolder
    topPipe.texturePath = "res/textures/pipetop.png"

    local botPipe = Instance.new("PhysicsSprite", "pipeBottom"..i)
    botPipe.bodyType = "static"
    botPipe.scale = vec2.new(0.5, 2.5)
    botPipe.parent = pipesHolder
    botPipe.texturePath = "res/textures/pipebottom.png"

    local halfPipeHeight = topPipe.scale.y / 2
    -- Adjust minY and maxY to respect both the pipes' sizes and desired gap range
    local minY = math.max(minGapY, -1 + gapSize/2 + halfPipeHeight)
    local maxY = math.min(maxGapY, 1 - gapSize/2 - halfPipeHeight)
    local gapY = math.random() * (maxY - minY) + minY

    topPipe.position = vec2.new(5 + (i - 1) * 3, gapY + gapSize/2 + halfPipeHeight)
    botPipe.position = vec2.new(5 + (i - 1) * 3, gapY - gapSize/2 - halfPipeHeight)

    table.insert(pipesHolder.data["pipes"], {top = topPipe, bottom = botPipe})
end

while true do
    for _, pair in pairs(pipesHolder.data["pipes"]) do
        pair.top.position = pair.top.position - vec2.new(0.02, 0)
        pair.bottom.position = pair.bottom.position - vec2.new(0.02, 0)
    end

    yield()
end
