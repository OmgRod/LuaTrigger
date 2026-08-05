-- not ready for use. code has been commented out for now

local startX = 90
local startY = 90

local spacing = 30
for y = 0, 9 do
    for x = 0, 9 do

        local id = math.random(1, 1000)

        local obj = Object.new(id)

        obj:addToLayer()

        obj:setPosition(
            startX + x * spacing,
            startY + y * spacing
        )
    end
end