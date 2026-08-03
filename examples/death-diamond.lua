-- use group 2 object

local i = 0

while i < 3 do
    Object.move(2, 60, 60, 0.5)
    wait(0.5)
    Object.move(2, 60, -60, 0.5)
    wait(0.5)
    Object.move(2, -60, -60, 0.5)
    wait(0.5)
    Object.move(2, -60, 60, 0.5)
    wait(0.5)
    i = i + 1
    if i > 2 then
        warn("last cycle done")
    else
        print("cycle done")
    end
end

error("player died")
Player.kill()