#include "LuaInterpreter.hpp"
#include <Geode/Geode.hpp>
#include <miskaa.notif/src/includes/notif_api.hpp>
#include <utils/Utils.hpp>

using namespace geode::prelude;

std::unordered_map<GJBaseGameLayer*, std::shared_ptr<LuaInterpreter>> LuaInterpreter::s_registry;

std::shared_ptr<LuaInterpreter> LuaInterpreter::forLayer(GJBaseGameLayer* layer) {
    if (!layer) return nullptr;

    auto it = s_registry.find(layer);
    if (it != s_registry.end()) {
        return it->second;
    }

    auto interp = std::make_shared<LuaInterpreter>();
    interp->init(layer);
    s_registry[layer] = interp;
    return interp;
}

void LuaInterpreter::cleanupLayer(GJBaseGameLayer* layer) {
    s_registry.erase(layer);
}

LuaInterpreter::LuaInterpreter() {
    m_lua = std::make_shared<sol::state>();
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

    m_lua->set_function("print", [](sol::variadic_args args) {
        auto text = formatLuaArgs(args);
        log::info("[Lua] {}", text);
        notifapi::info(text);
    });

    m_lua->set_function("error", [](sol::variadic_args args) {
        auto text = formatLuaArgs(args);
        log::error("[Lua] {}", text);
        notifapi::error(text);
    });

    m_lua->set_function("warn", [](sol::variadic_args args) {
        auto text = formatLuaArgs(args);
        log::warn("[Lua] {}", text);
        notifapi::warn(text);
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

sol::environment LuaInterpreter::createScriptEnvironment() {
    sol::environment env(*m_lua, sol::create);

    env["state"] = m_persistentState;

    auto meta = m_lua->create_table();
    meta["__index"] = m_lua->globals();
    env[sol::metatable_key] = meta;

    return env;
}

void LuaInterpreter::runCoroutine(std::shared_ptr<sol::coroutine> coroutine, sol::environment env, uint32_t token) {
    if (m_disabled || !m_lua || !coroutine->runnable() || token != m_executionToken) {
        return;
    }

    auto result = (*coroutine)(env);

    if (!result.valid()) {
        sol::error err = result;
        log::error("Lua Coroutine Error: {}", err.what());
        return;
    }

    if (m_disabled || !m_lua || !coroutine->runnable() || token != m_executionToken) return;

    if (result.return_count() > 0) {
        float seconds = result.get<float>(0);

        cocos2d::CCNode* targetNode = PlayLayer::get();
        if (!targetNode) {
            targetNode = cocos2d::CCDirector::sharedDirector()->getRunningScene();
        }

        if (!targetNode) return;

        targetNode->runAction(
            cocos2d::CCSequence::create(
                cocos2d::CCDelayTime::create(seconds),
                CallFuncExt::create(
                    [this, coroutine, env, token]() mutable {
                        if (m_disabled || !m_lua || token != m_executionToken) {
                            return;
                        }
                        runCoroutine(coroutine, env, token);
                    }
                ),
                nullptr
            )
        );
    }
}

bool LuaInterpreter::runString(const std::string& code) {
    if (!m_lua) return false;

    m_disabled = false;

    sol::environment env = createScriptEnvironment();

    sol::load_result loaded = m_lua->load(code);
    if (!loaded.valid()) {
        sol::error err = loaded;
        log::error("Lua Load Error: {}", err.what());
        return false;
    }

    sol::protected_function fn = loaded;
    env.set_on(fn);

    auto coro = std::make_shared<sol::coroutine>(fn);
    uint32_t token = m_executionToken;

    runCoroutine(coro, env, token);
    return true;
}

void LuaInterpreter::stop() {
    m_disabled = true;
    m_executionToken++;
}

void LuaInterpreter::pause() {
    m_disabled = true;
}

void LuaInterpreter::resume() {
    m_disabled = false;
}

void LuaInterpreter::resetState() {
    if (!m_lua) return;
    m_disabled = false;
    m_executionToken++;
    m_persistentState = m_lua->create_table();
    (*m_lua)["state"] = m_persistentState;
}

void LuaInterpreter::bindEngineAPI(GJBaseGameLayer* layer) {
    if (!layer || !m_lua) return;

    sol::table playerTable = m_lua->create_named_table("Player",
        "Player1", 1,
        "Player2", 2,
        "Both", 3
    );

    auto getPlayer = [layer](int type) -> PlayerObject* {
        if (type == 2) return layer->m_player2;
        return layer->m_player1;
    };

    playerTable["kill"] = [layer](sol::optional<int> playerType) {
        int type = playerType.value_or(1);
        auto kill = [&](PlayerObject* p) { if (p) layer->destroyPlayer(p, nullptr); };
        if (type == 1 || type == 3) kill(layer->m_player1);
        if (type == 2 || type == 3) kill(layer->m_player2);
    };

    playerTable["flipGravity"] = [getPlayer](sol::optional<int> playerType, sol::optional<bool> noEffects) {
        int type = playerType.value_or(1);
        bool fx = !noEffects.value_or(false);
        if (auto* p = getPlayer(type)) p->flipGravity(!p->m_isUpsideDown, !fx);
    };

    playerTable["setGravity"] = [getPlayer](sol::optional<int> playerType, bool upsideDown, sol::optional<bool> noEffects) {
        int type = playerType.value_or(1);
        bool fx = !noEffects.value_or(false);
        if (auto* p = getPlayer(type)) p->flipGravity(upsideDown, !fx);
    };

    playerTable["setYVelocity"] = [getPlayer](sol::optional<int> playerType, double velocity) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->setYVelocity(velocity, 1);
    };

    playerTable["addYVelocity"] = [getPlayer](sol::optional<int> playerType, double yVelocity) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->addToYVelocity(yVelocity, 1);
    };

    playerTable["setVisible"] = [getPlayer](sol::optional<int> playerType, bool visible) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->toggleVisibility(visible);
    };

    playerTable["enableControls"] = [getPlayer](sol::optional<int> playerType) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->enablePlayerControls();
    };

    playerTable["disableControls"] = [getPlayer](sol::optional<int> playerType) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->disablePlayerControls();
    };

    playerTable["setSpeed"] = [getPlayer](sol::optional<int> playerType, bool faster) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) {
            if (faster) p->speedUp();
            else p->speedDown();
        }
    };

    playerTable["gravityUp"] = [getPlayer](sol::optional<int> playerType) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->gravityUp();
    };

    playerTable["gravityDown"] = [getPlayer](sol::optional<int> playerType) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->gravityDown();
    };

    playerTable["boostPlayer"] = [getPlayer](sol::optional<int> playerType, float yVelocity) {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) p->boostPlayer(yVelocity);
    };

    playerTable["getX"] = [getPlayer](sol::optional<int> playerType) -> float {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->getPositionX();
        return 0.f;
    };

    playerTable["getY"] = [getPlayer](sol::optional<int> playerType) -> float {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->getPositionY();
        return 0.f;
    };

    playerTable["getPosition"] = [getPlayer](sol::optional<int> playerType) -> std::tuple<float, float> {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return { p->getPositionX(), p->getPositionY() };
        return { 0.f, 0.f };
    };

    playerTable["getYVelocity"] = [getPlayer](sol::optional<int> playerType) -> double {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->getYVelocity();
        return 0.0;
    };

    playerTable["getXVelocity"] = [getPlayer](sol::optional<int> playerType) -> double {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->getCurrentXVelocity();
        return 0.0;
    };

    playerTable["getRotation"] = [getPlayer](sol::optional<int> playerType) -> float {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->getRotation();
        return 0.f;
    };

    playerTable["getScale"] = [getPlayer](sol::optional<int> playerType) -> float {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->getScale();
        return 1.f;
    };

    playerTable["getGravity"] = [getPlayer](sol::optional<int> playerType) -> double {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_gravity;
        return 0.0;
    };

    playerTable["getSpeedMultiplier"] = [getPlayer](sol::optional<int> playerType) -> double {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_speedMultiplier;
        return 1.0;
    };

    playerTable["getTotalTime"] = [getPlayer](sol::optional<int> playerType) -> double {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_totalTime;
        return 0.0;
    };

    playerTable["isDead"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isDead;
        return false;
    };

    playerTable["isUpsideDown"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isUpsideDown;
        return false;
    };

    playerTable["isOnGround"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isOnGround;
        return false;
    };

    playerTable["isGoingLeft"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isGoingLeft;
        return false;
    };

    playerTable["isSideways"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isSideways;
        return false;
    };

    playerTable["isDashing"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isDashing;
        return false;
    };

    playerTable["isMoving"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isMoving;
        return false;
    };

    playerTable["isFlying"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->isFlying();
        return false;
    };

    playerTable["isLocked"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isLocked;
        return false;
    };

    playerTable["hasEverJumped"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_hasEverJumped;
        return false;
    };

    playerTable["isPlatformer"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isPlatformer;
        return false;
    };

    playerTable["isOnSlope"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isOnSlope;
        return false;
    };

    playerTable["isSliding"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isSliding;
        return false;
    };

    playerTable["isAccelerating"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isAccelerating;
        return false;
    };

    playerTable["isHidden"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isHidden;
        return false;
    };

    playerTable["hasGlow"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_hasGlow;
        return false;
    };

    playerTable["isSwing"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isSwing;
        return false;
    };

    playerTable["isShip"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isShip;
        return false;
    };

    playerTable["isBird"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isBird;
        return false;
    };

    playerTable["isBall"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isBall;
        return false;
    };

    playerTable["isDart"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isDart;
        return false;
    };

    playerTable["isRobot"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isRobot;
        return false;
    };

    playerTable["isSpider"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isSpider;
        return false;
    };

    playerTable["isCube"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type))
            return !p->m_isShip && !p->m_isBird && !p->m_isBall && !p->m_isDart
                && !p->m_isRobot && !p->m_isSpider && !p->m_isSwing;
        return false;
    };

    playerTable["getMode"] = [getPlayer](sol::optional<int> playerType) -> std::string {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) {
            if (p->m_isShip)   return "ship";
            if (p->m_isBird)   return "ufo";
            if (p->m_isBall)   return "ball";
            if (p->m_isDart)   return "wave";
            if (p->m_isRobot)  return "robot";
            if (p->m_isSpider) return "spider";
            if (p->m_isSwing)  return "swing";
            return "cube";
        }
        return "unknown";
    };

    playerTable["isHoldingLeft"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_holdingLeft;
        return false;
    };

    playerTable["isHoldingRight"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_holdingRight;
        return false;
    };

    playerTable["getPlatformerXVelocity"] = [getPlayer](sol::optional<int> playerType) -> double {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_platformerXVelocity;
        return 0.0;
    };

    playerTable["isSecondPlayer"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isSecondPlayer;
        return false;
    };

    playerTable["isOutOfBounds"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_isOutOfBounds;
        return false;
    };

    playerTable["getVehicleSize"] = [getPlayer](sol::optional<int> playerType) -> float {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_vehicleSize;
        return 1.f;
    };

    playerTable["getPlayerSpeed"] = [getPlayer](sol::optional<int> playerType) -> float {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_playerSpeed;
        return 0.f;
    };

    playerTable["getLastPortalPos"] = [getPlayer](sol::optional<int> playerType) -> std::tuple<float, float> {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return { p->m_lastPortalPos.x, p->m_lastPortalPos.y };
        return { 0.f, 0.f };
    };

    playerTable["getLastGroundedPos"] = [getPlayer](sol::optional<int> playerType) -> std::tuple<float, float> {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return { p->m_lastGroundedPos.x, p->m_lastGroundedPos.y };
        return { 0.f, 0.f };
    };

    playerTable["getGravityMod"] = [getPlayer](sol::optional<int> playerType) -> float {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_gravityMod;
        return 1.f;
    };

    playerTable["hasTouchedRing"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_touchedRing;
        return false;
    };

    playerTable["hasTouchedPad"] = [getPlayer](sol::optional<int> playerType) -> bool {
        int type = playerType.value_or(1);
        if (auto* p = getPlayer(type)) return p->m_touchedPad;
        return false;
    };

    sol::table popupTable = m_lua->create_named_table("Popup");

    popupTable["show"] = [layer](std::string title, std::string content, sol::optional<std::string> btn1, sol::optional<std::string> btn2, sol::optional<sol::function> callback, sol::optional<bool> doShow, sol::optional<bool> cancelledByEscape) {
        std::shared_ptr<sol::protected_function> luaCallback;

        if (callback.has_value() && callback->valid()) {
            luaCallback = std::make_shared<sol::protected_function>(*callback);
        }

        togglePlayerMovement(layer, true);

        PlatformToolbox::showCursor();

        std::function<void(FLAlertLayer*, bool)> callbackWrapper =
            [luaCallback, layer](FLAlertLayer* popup, bool secondButton) {
                togglePlayerMovement(layer, false);

                if (!GameManager::sharedState()->getGameVariable("gv_0024")) PlatformToolbox::hideCursor();

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
        auto array = CCArray::create();

        for (size_t i = 1; i <= dialogList.size(); ++i) {
            sol::optional<sol::table> entryOpt = dialogList[i];
            if (!entryOpt.has_value()) continue;

            auto entry = entryOpt.value();

            std::string speaker = entry.get_or("speaker", std::string(""));
            std::string text    = entry.get_or("text", std::string(""));
            int avatar          = entry.get_or("avatar", 1);
            float scale         = entry.get_or("scale", 1.0f);
            bool unskippable    = entry.get_or("unskippable", false);

            ccColor3B color = { 255, 255, 255 };
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

    sol::table itemTable = m_lua->create_named_table("Item",
        "item", 1,
        "timer", 2,
        "points", 3
    );

    itemTable["get"] = [layer](int type, sol::optional<int> id) -> double {
        int targetId = id.value_or(0);
        return layer->getItemValue(type, targetId);
    };

    itemTable["set"] = [layer](int itemID, double targetValue) {
        double currentValue = layer->getItemValue(1, itemID);
        double diff = targetValue - currentValue;
        if (diff == 0) return;

        auto pickupTrigger = static_cast<EffectGameObject*>(GameObject::createWithKey(1817));
        if (!pickupTrigger) {
            log::error("Pickup trigger creation failed");
            return;
        }

        pickupTrigger->m_itemID = itemID;
        if (diff < 0) {
            pickupTrigger->m_subtractCount = true;
            pickupTrigger->m_collectiblePoints = static_cast<int>(std::abs(diff));
        } else {
            pickupTrigger->m_subtractCount = false;
            pickupTrigger->m_collectiblePoints = static_cast<int>(diff);
        }

        layer->pickupItem(pickupTrigger);
    };
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
