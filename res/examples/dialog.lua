-- Example: Shop Entry Dialogue converted to LuaTrigger bindings

Dialog.show({
    {
        speaker = "Scratch",
        text = "<d030>Hello player!",
        avatar = 8,
        scale = 1.0,
        unskippable = false
    },
    {
        speaker = "Scratch",
        text = "<co>The Shopkeeper</c> asked me to take care of the shop.",
        avatar = 9
    },
    {
        speaker = "The Shopkeeper",
        text = "<s100>HEY!</s> What are <cr>you</c> doing here?!",
        avatar = 30
    },
    {
        speaker = "Scratch",
        text = "I'm just here to <cg>buy</c> something.",
        avatar = 26
    },
    {
        speaker = "The Shopkeeper",
        text = "Oh, okay. I've got something to deal with.",
        avatar = 5
    },
    {
        speaker = "The Shopkeeper",
        text = "<d030>You lot better not <cr>mess around</c>!",
        avatar = 5
    },
    {
        speaker = "Scratch",
        text = "Don't worry! We won't do anything <cg>bad</c>!",
        avatar = 10
    },
    {
        speaker = "Scratch",
        text = "Keep this between us.",
        avatar = 13
    },
    {
        speaker = "Scratch",
        text = "I'm actually here to <cr>steal</c> some stuff.",
        avatar = 13
    },
    {
        speaker = "Scratch",
        text = "From <co>The Shopkeeper</c>.",
        avatar = 13
    },
    {
        speaker = "Scratch",
        text = "Wanna join me?",
        avatar = 13
    },
    {
        speaker = "Scratch",
        text = "No? <d040>You scared or something?",
        avatar = 14
    },
    {
        speaker = "Scratch",
        text = "<d030>Nevermind. I'll <co>do it myself</c>.",
        avatar = 13
    }
}, {
    bgType = 2,
    animateSide = true
})