local decorations = {}

for _, child in pairs(script.parent.children) do
	if child:isA("PhysicsSprite") or child:isA("Sprite") then
		table.insert(decorations, child)
	end
end

local speed = 0.02
local screenLeft = -3.95
local screenRight = 3.95

while true do
	for _, land in pairs(decorations) do
		-- Scroll left
		land.position = land.position - vec2.new(speed, 0)

		-- Wrap: if completely off-screen left, move it just past the right edge
		if land.position.x + land.scale.x / 2 < screenLeft then
			land.position = vec2.new(screenRight + land.scale.x / 2, land.position.y)
		end
	end

	yield()
end