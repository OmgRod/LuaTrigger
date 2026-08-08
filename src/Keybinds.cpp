#include <Geode/Geode.hpp>
#include <DebugConsole.hpp>
#include <utils/LogManager.hpp>

using namespace geode::prelude;

$on_game(Loaded) {
    LogManager::get().addCallback([](const LogEntry& entry) {
        if (auto console = DebugConsole::get()) {
            console->addLogEntry(entry);
        }
    });

    listenForKeybindSettingPresses("debug-console", [](Keybind const& keybind, bool down, bool repeat, double timestamp) {
        if (down && !repeat) {
            auto om = OverlayManager::get();
            if (!om) return;
            
            if (auto existing = DebugConsole::get()) {
                existing->removeFromParent();
            } else {
                auto newc = DebugConsole::create();
                if (newc) {
                    om->addChild(newc);
                }
            }
        }
    });

    listenForKeybindSettingPresses("clear-console", [](Keybind const& keybind, bool down, bool repeat, double timestamp) {
        if (down && !repeat) {
            LogManager::get().clearLogs();
            if (auto console = DebugConsole::get()) {
                console->updateContent();
            }
        }
    });
};
