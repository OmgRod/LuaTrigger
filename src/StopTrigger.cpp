#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <ExecuteLuaTrigger.hpp>
#include <LuaInterpreter.hpp>
#include <utils/Utils.hpp>

using namespace geode::prelude;

// this bit handles the stop trigger fix

class $modify(MyGameLayer, GJBaseGameLayer) {
    void controlTriggersInGroup(int groupID, GJActionCommand command) {
        GJBaseGameLayer::controlTriggersInGroup(groupID, command);

        auto groupObjects = this->getGroup(groupID);
        if (!groupObjects) return;

        for (auto* obj : CCArrayExt<CCObject*>(groupObjects)) {
            if (auto* luaTrigger = typeinfo_cast<ExecuteLuaTrigger*>(obj)) {
                log::info("Control command {} received for group {}", static_cast<int>(command), groupID);

                if (command == GJActionCommand::Stop) {
                    log::info("Stopping LuaTrigger!");
                    luaTrigger->stopLua();
                } else if (command == GJActionCommand::Pause) {
                    log::info("Pausing LuaTrigger!");
                    luaTrigger->pauseLua();
                } else if (command == GJActionCommand::Resume) {
                    log::info("Resuming LuaTrigger!");
                    luaTrigger->resumeLua();
                }
            }
        }
    }

    void onExit() {
        LuaInterpreter::cleanupLayer(this);
        GJBaseGameLayer::onExit();
    }
};

// this bit handles the player death/level restart fix

class $modify(MyPlayLayer, PlayLayer) {
    void resetLevel() {
        LevelResetEvent().send();
        PlayLayer::resetLevel();
    }
};
