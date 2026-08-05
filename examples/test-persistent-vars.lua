-- Not really an example, but good to keep!
-- I guess you can use this if you don't know how to use persistent variables

print("=== Lua Test Started ===")

print("Normal variable before:")
print(normalCounter)

normalCounter = (normalCounter or 0) + 1

print("Normal variable after increment:")
print(normalCounter)


print("Persistent variable before:")
print(state.counter)

state.counter = (state.counter or 0) + 1

print("Persistent variable after increment:")
print(state.counter)


print("Waiting 1 second...")
wait(1)

print("Finished waiting!")

print("Testing another persistent value")

state.message = "Hello from persistent state"

print("Stored message:")
print(state.message)


print("Creating temporary function")

function temporaryFunction()
    print("This function only exists this run")
end

temporaryFunction()


print("=== Lua Test Finished ===")