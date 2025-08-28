local camera = root:findChildByName("MainCamera")
local cursor = script.parent

local calls = 0
local textureIndex = 1

while true do
	engine:hideCursor()

	local mousePos = engine:getMousePos()
	local worldPos = camera:screenToWorld(mousePos)

	cursor.position = worldPos
	cursor.texturePath = "cursor"..textureIndex..".png"

	calls = calls + 1
	if calls >= 20 then
		textureIndex = textureIndex + 1
		calls = 0
	end

	if textureIndex > 3 then
		textureIndex = 1
	end

	yield()
end