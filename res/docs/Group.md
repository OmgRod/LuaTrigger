# Object Control

The `Group` class provides functions to interact with and manipulate the game objects in real time.

---

## Functions

### `Group.move([groupID], [dx], [dy], [duration])`

Moves a specific object group by `dx` editor units in the x-axis and `dy` units in the y-axis over `duration` (in seconds) time.

#### Arguments

* **`groupID`** *(required, integer)*: Determines which group of objects to move. This parameter is required.
* **`dx`** *(required, integer)*: Determines which group of objects to move. This parameter is required.
* **`dy`** *(required, integer)*: Determines which group of objects to move. This parameter is required.
* **`duration`** *(optional, integer)*: How long you should spend 

## Code Examples

```lua
-- Kills Player 1 (Default behavior)
Player.kill()

-- Kills Player 2 explicitly
Player.kill(2)

-- Kills both players simultaneously (Dual Mode)
Player.kill(Player.Both) -- or Player.kill(3)
```

In most cases, you will want to only kill Player 1, so you can just leave out the parameter.

---

