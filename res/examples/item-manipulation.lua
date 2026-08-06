-- Item manipulation example using LuaTrigger

-- Get current value of Item ID 1
local currentCoins = Item.get(Item.item, 1)
print("Current coins (Item 1): " .. tostring(currentCoins))

-- Set Item ID 1 to 50
Item.set(1, 50)

local newCoins = Item.get(Item.item, 1)
print("New coins (Item 1): " .. tostring(newCoins))

-- Get value of Timer ID 1 and Points
local timerVal = Item.get(Item.timer, 1)
local pointsVal = Item.get(Item.points)

print("Timer 1 value: " .. tostring(timerVal))
print("Points value: " .. tostring(pointsVal))
