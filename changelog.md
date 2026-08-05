# Changelog

## v1.0.0-alpha.6

- Removed `LuaManager` and moved everything from it into `LuaTrigger`
- From above change, fixed bug with Stop triggers not working on the Lua trigger
- Made a separate fix to stop the trigger when the player dies
- Added `Dialog.show()` function
- Added `easingType` and `easingRate` to move command
- Fixed the syntax highlighter's bugs:
  - Not highlighting as soon as the file is uploaded
  - Stuff being highlighted despite being commented out

## v1.0.0-alpha.5

- Added amber back as a dependency
- Fixed issue with file paths
- Completely removed support for `LevelEditorLayer` because it's very strange

## v1.0.0-alpha.4

- No actual changes
- Just used this release to get someone's attention :)

## v1.0.0-alpha.3

- Fixed utils buttons alignment
- Added `Popup.show()` function
- Made code more consistent and tidy
- Add iOS support
- Switched to a new logo

## v1.0.0-alpha.2

- Updated the mod's metadata

## v1.0.0-alpha.1

- Added `Player.kill()` function
- Added `Object.move()` function
- Added `Object.rotate()` function
- Added `Object.scale()` function
- Added `wait()` function
- Added notifications to `print()`, `warn()`, and `error()` functions
