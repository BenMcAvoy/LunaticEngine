local bird = script.parent

local hitSFX = bird:findChildByName("HitSFX")
local wingSFX = bird:findChildByName("WingSFX")

local alive = true

bird:setOnCollisionEnter(function (self, other)
	alive = false
	hitSFX:play()
end)

while true do
	while not alive do
		engine:drawText(vec2.new(0, 0.95), "You died", vec4.new(0.0, 0.0, 0.0, 1))
		engine:drawText(vec2.new(0, 0.9), "Press space to restart", vec4.new(0.0, 0.0, 0.0, 1))
		yield()
	end

	if engine:getKeyState(keys.space) == 1 then
		bird.linearVelocity = vec2.new(0, 3)
		wingSFX:play()
	end

	yield()
end