-- opens popup

Popup.show(
    "Hello",
    "Do you want to continue?",
    "No",
    "Yes",
    function(popup, yes)
        if yes then
            print("Pressed Yes")
        else
            print("Pressed No")
        end
    end
)