#include "LuaInterpreter.hpp"
#include "ExecuteLuaTrigger.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

LuaInterpreter::LuaInterpreter() {
    m_lua = std::make_unique<sol::state>();
}

void LuaInterpreter::init(GJBaseGameLayer* layer) {
    if (m_initialized) return;

    m_lua->open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::bit32,
        sol::lib::coroutine,
        sol::lib::table,
        sol::lib::utf8
    );

    m_persistentState = m_lua->create_table();
    (*m_lua)["state"] = m_persistentState;

    m_lua->set_function("wait", [](double seconds, sol::this_state s) {
        lua_pushnumber(s, seconds);
        return lua_yield(s, 1);
    });

    m_lua->set_function("print", [this](sol::variadic_args args) {
        auto text = formatLuaArgs(args);
        log::info("[Lua] {}", text);
        Notification::create(text, NotificationIcon::Info)->show();
    });

    m_lua->set_function("error", [this](sol::variadic_args args) {
        auto text = formatLuaArgs(args);
        log::error("[Lua] {}", text);
        Notification::create(text, NotificationIcon::Error)->show();
    });

    m_lua->set_function("warn", [this](sol::variadic_args args) {
        auto text = formatLuaArgs(args);
        log::warn("[Lua] {}", text);
        Notification::create(text, NotificationIcon::Warning)->show();
    });

    m_lua->set_function("clearState", [this]() {
        resetState();
    });

    if (layer) {
        bindEngineAPI(layer);
    } else if (auto playLayer = PlayLayer::get()) {
        bindEngineAPI(playLayer);
    }

    m_initialized = true;
}

void LuaInterpreter::bindEngineAPI(GJBaseGameLayer* layer) {
    if (!layer || !m_lua) return;

    sol::table playerTable = m_lua->create_named_table("Player", 
        "Player1", 1,
        "Player2", 2,
        "Both", 3
    );

    playerTable["kill"] = [layer](sol::optional<int> playerType) {
        int type = playerType.value_or(1);

        auto killPlayer = [&](PlayerObject* p) {
            if (p) layer->destroyPlayer(p, nullptr);
        };

        if (type == 1 || type == 3) killPlayer(layer->m_player1);
        if (type == 2 || type == 3) killPlayer(layer->m_player2);
    };

    sol::table popupTable = m_lua->create_named_table("Popup");
    popupTable["show"] = [layer](std::string title, std::string content, 
                                sol::optional<std::string> btn1, 
                                sol::optional<std::string> btn2, 
                                sol::optional<sol::function> callback, 
                                sol::optional<bool> doShow, 
                                sol::optional<bool> cancelledByEscape) {
        std::shared_ptr<sol::protected_function> luaCallback;

        if (callback.has_value() && callback->valid()) {
            luaCallback = std::make_shared<sol::protected_function>(*callback);
        }

        togglePlayerMovement(layer, true);

        std::function<void(FLAlertLayer*, bool)> callbackWrapper =
            [luaCallback, layer](FLAlertLayer* popup, bool secondButton) {
                togglePlayerMovement(layer, false);

                if (!luaCallback) return;

                auto result = (*luaCallback)(popup, secondButton);

                if (!result.valid()) {
                    sol::error err = result;
                    log::error("Lua popup callback error: {}", err.what());
                }
            };

        std::string button1 = btn1.value_or("OK");
        std::string button2 = btn2.value_or("");

        if (button2.empty()) {
            return createQuickPopup(
                title.c_str(),
                content,
                button1.c_str(),
                nullptr,
                callbackWrapper,
                doShow.value_or(true),
                cancelledByEscape.value_or(false)
            );
        }

        return createQuickPopup(
            title.c_str(),
            content,
            button1.c_str(),
            button2.c_str(),
            callbackWrapper,
            doShow.value_or(true),
            cancelledByEscape.value_or(false)
        );
    };

    sol::table dialogTable = m_lua->create_named_table("Dialog");
    dialogTable["show"] = [layer](sol::table dialogList, sol::optional<sol::table> options) {
        auto array = cocos2d::CCArray::create();

        for (size_t i = 1; i <= dialogList.size(); ++i) {
            sol::optional<sol::table> entryOpt = dialogList[i];
            if (!entryOpt.has_value()) continue;

            auto entry = entryOpt.value();

            std::string speaker = entry.get_or("speaker", std::string(""));
            std::string text    = entry.get_or("text", std::string(""));
            int avatar          = entry.get_or("avatar", 1);
            float scale         = entry.get_or("scale", 1.0f);
            bool unskippable    = entry.get_or("unskippable", false);

            cocos2d::ccColor3B color = { 255, 255, 255 };
            if (sol::optional<sol::table> colorTbl = entry["color"]) {
                color.r = colorTbl->get_or(1, 255);
                color.g = colorTbl->get_or(2, 255);
                color.b = colorTbl->get_or(3, 255);
            }

            auto obj = DialogObject::create(
                speaker.c_str(),
                text.c_str(),
                avatar,
                scale,
                unskippable,
                color
            );

            if (obj) {
                array->addObject(obj);
            }
        }

        if (array->count() == 0) return;

        int bgType = 2;
        bool animateSide = true;

        if (options.has_value()) {
            bgType = options->get_or("bgType", 2);
            animateSide = options->get_or("animateSide", true);
        }

        togglePlayerMovement(layer, true);

        auto dialogLayer = DialogLayer::createWithObjects(array, bgType);
        if (!dialogLayer) return;

        if (animateSide) {
            dialogLayer->animateInRandomSide();
        }

        auto cleanupNode = DialogCleanupNode::create(layer);
        dialogLayer->addChild(cleanupNode);

        dialogLayer->addToMainScene();
    };

    sol::table groupTable = m_lua->create_named_table("Object");
    
    groupTable["move"] = [layer](int groupID, float dx, float dy, sol::optional<float> duration, sol::optional<int> easingType, sol::optional<float> easingRate) {
        log::info("Lua Object.move({}, {}, {}, {})",
            groupID, dx, dy, duration.value_or(0.f)
        );

        moveGroupWithEasing(layer, groupID, { dx, dy }, duration.value_or(0.0f), easingType.value_or(0), easingRate.value_or(2.0f));
    };

    groupTable["rotate"] = [layer](int targetGroupID, int centerGroupID, int degrees, sol::optional<int> times360, sol::optional<float> duration, sol::optional<int> easingType, sol::optional<float> easingRate) {
        log::info("Lua Object.rotate({}, {}, {}, {})", targetGroupID, centerGroupID, degrees, duration.value_or(0.f));

        rotateGroupWithEasing(layer, targetGroupID, centerGroupID, degrees, times360.value_or(0), duration.value_or(0.0f), easingType.value_or(0), easingRate.value_or(2.0f));
    };

    groupTable["scale"] = [layer](int targetGroupID, int centerGroupID, float scaleX, sol::optional<float> scaleY, sol::optional<float> duration, sol::optional<int> easingType, sol::optional<float> easingRate, sol::optional<sol::table> options) {
        log::info("Lua Group.scale({}, {}, {}, {})", targetGroupID, centerGroupID, scaleX, duration.value_or(0.f));

        float sy = scaleY.value_or(scaleX);
        float dur = duration.value_or(0.0f);

        bool divByX = false;
        bool divByY = false;
        bool onlyMove = false;
        bool relativeScale = false;
        bool relativeRotation = false;

        if (options.has_value()) {
            auto opts = options.value();
            divByX = opts.get_or("divByX", false);
            divByY = opts.get_or("divByY", false);
            onlyMove = opts.get_or("onlyMove", false);
            relativeScale = opts.get_or("relativeScale", false);
            relativeRotation = opts.get_or("relativeRotation", false);
        }

        scaleGroupWithEasing(layer, targetGroupID, centerGroupID, scaleX, sy, dur, easingType.value_or(0), easingRate.value_or(2.0f), divByX, divByY, onlyMove, relativeScale, relativeRotation);
    };
}

bool LuaInterpreter::runString(const std::string& code) {
    if (!m_lua) return false;

    auto result = m_lua->script(code, [](lua_State*, sol::protected_function_result pfr) {
        return pfr;
    });

    if (!result.valid()) {
        sol::error err = result;
        geode::log::error("Lua Execution Error: {}", err.what());
        return false;
    }

    return true;
}

void LuaInterpreter::resetState() {
    if (!m_lua) return;
    m_persistentState = m_lua->create_table();
    (*m_lua)["state"] = m_persistentState;
}

std::string LuaInterpreter::formatLuaArgs(sol::variadic_args args) {
    std::string result;
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (it != args.begin()) result += " ";
        
        sol::object obj = *it;
        if (obj.is<std::string>()) {
            result += obj.as<std::string>();
        } else if (obj.is<bool>()) {
            result += obj.as<bool>() ? "true" : "false";
        } else if (obj.is<double>()) {
            result += std::to_string(obj.as<double>());
        } else {
            result += "[object]";
        }
    }
    return result;
}
