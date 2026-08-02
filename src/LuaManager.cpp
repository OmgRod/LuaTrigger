#include "LuaManager.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

void moveGroupWithEasing(
    GJBaseGameLayer* gameLayer, 
    int targetGroupID, 
    cocos2d::CCPoint offset, 
    float duration, 
    int easingType = 0, 
    float easingRate = 2.0f
) {
    if (!gameLayer) return;
    
    auto moveTrigger = static_cast<EffectGameObject*>(GameObject::createWithKey(901));
    if (!moveTrigger) {
        log::error("Move trigger cast failed");
        return;
    }

    moveTrigger->m_targetGroupID = targetGroupID;
    moveTrigger->m_moveOffset = offset;
    moveTrigger->m_duration = duration;
    
    moveTrigger->m_easingType = static_cast<EasingType>(easingType);
    moveTrigger->m_easingRate = easingRate;

    gameLayer->triggerMoveCommand(moveTrigger);
}

LuaManager::LuaManager() {
    m_lua = std::make_unique<sol::state>();
    m_lua->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
    
    sol::table playerTable = m_lua->create_named_table("Player", 
        "Player1", 1,
        "Player2", 2,
        "Both", 3
    );

    playerTable["kill"] = [](sol::optional<int> playerType) {
        int type = playerType.value_or(1);
        
        GJBaseGameLayer* layer = nullptr;
        if (auto playLayer = PlayLayer::get()) {
            layer = playLayer;
        } else if (auto editorLayer = LevelEditorLayer::get()) {
            layer = editorLayer;
        }

        if (!layer) return;

        auto killPlayer = [&](PlayerObject* p) {
            if (p) layer->destroyPlayer(p, nullptr);
        };

        if (type == 1 || type == 3) killPlayer(layer->m_player1);
        if (type == 2 || type == 3) killPlayer(layer->m_player2);
    };

    sol::table groupTable = m_lua->create_named_table("Group");
    
    groupTable["move"] = [](int groupID, float dx, float dy, sol::optional<float> duration) {
        GJBaseGameLayer* layer = nullptr;
        if (auto playLayer = PlayLayer::get()) layer = playLayer;
        else if (auto editorLayer = LevelEditorLayer::get()) layer = editorLayer;
        if (!layer) return;

        float dur = duration.value_or(0.0f);

        moveGroupWithEasing(layer, groupID, { dx, dy }, dur);
    };
}

void LuaManager::executeScript(const std::string& code) {
    if (!m_lua) return;

    auto result = m_lua->safe_script(code, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        log::error("Lua Execution Error: {}", err.what());
    }
}