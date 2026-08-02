# Player Control

The `Player` class provides functions to interact with and manipulate the player state in real time.

---

## Functions

### `Player.kill([playerType])`

Kills the specified player(s) in the active level.

#### Arguments

* **`playerType`** *(optional, integer)*: Determines which player object to destroy. Default is `1`.

| Value | Target |
| :--- | :--- |
| `1` | Player 1 |
| `2` | Player 2 |
| `3` | Both Player 1 & Player 2 |

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

