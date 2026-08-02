#include "LuaManager.hpp"
#include "LuaObject.hpp"

using namespace geode::prelude;

std::string formatLuaArgs(sol::variadic_args args) {
    std::string result;
    for (auto arg : args) {
        switch (arg.get_type()) {
            case sol::type::string:
                result += arg.as<std::string>();
                break;
            case sol::type::number:
                result += std::to_string(arg.as<double>());
                break;
            case sol::type::boolean:
                result += arg.as<bool>() ? "true" : "false";
                break;
            case sol::type::lua_nil:
                result += "nil";
                break;
            default:
                result += "<" + std::string(sol::type_name(args.lua_state(), arg.get_type())) + ">";
                break;
        }
        result += "\t";
    }
    return result;
}

bool isKeyword(const std::string& word) {
    static const std::unordered_set<std::string> keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function", 
        "if", "in", "local", "nil", "not", "or", "repeat", "return", "then", 
        "true", "until", "while"
    };
    return keywords.find(word) != keywords.end();
}

bool isFunction(const std::string& word, const std::string& code, size_t pos) {
    while(pos < code.size() && std::isspace(code[pos])) pos++;
    return pos < code.size() && code[pos] == '(';
}

bool isClass(const std::string& word) {
    return !word.empty() && std::isupper(word[0]);
}

std::string LuaManager::highlightSyntax(const std::string& code) {
    std::string result;
    size_t i = 0;
    while(i < code.size()) {
        if (code[i] == '"' || code[i] == '\'') {
            char quote = code[i];
            result += fmt::format("<c#FFFACD>{}", quote);
            i++;
            while(i < code.size() && code[i] != quote) {
                result += code[i];
                i++;
            }
            if(i < code.size()) { result += code[i]; i++; }
            result += "</c>";
            continue;
        }

        if (std::isalpha(code[i]) || code[i] == '_') {
            std::string word;
            size_t start = i;
            while(i < code.size() && (std::isalnum(code[i]) || code[i] == '_')) {
                word += code[i];
                i++;
            }

            if (isKeyword(word)) {
                result += fmt::format("<c#ADD8E6>{}</c>", word);
            } else if (isFunction(word, code, i)) {
                result += fmt::format("<c#90EE90>{}</c>", word);
            } else if (isClass(word)) {
                result += fmt::format("<c#FFB6C1>{}</c>", word);
            } else {
                result += word;
            }
            continue;
        }

        if (std::isdigit(code[i])) {
            std::string num;
            while(i < code.size() && (std::isdigit(code[i]) || code[i] == '.')) {
                num += code[i];
                i++;
            }
            result += fmt::format("<c#FFDAB9>{}</c>", num);
            continue;
        }

        result += code[i];
        i++;
    }
    return result;
}

LuaManager::LuaManager() {
    m_lua = std::make_unique<sol::state>();
    m_lua->open_libraries( // THIS MUST BE APPROPRIATE PACKAGES ONLY! WE DONT WANT OUR USERS GETTING HACKED!
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::bit32,
        sol::lib::coroutine,
        sol::lib::table,
        sol::lib::utf8
    );

    m_persistentState = m_lua->create_table();
    m_lua->globals()["state"] = m_persistentState;

    GJBaseGameLayer* layer = nullptr;
    if (auto playLayer = PlayLayer::get()) layer = playLayer;
    else if (auto editorLayer = LevelEditorLayer::get()) layer = editorLayer;
    if (!layer) return;
    
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

    sol::table groupTable = m_lua->create_named_table("Object");
    
    groupTable["move"] = [layer](int groupID, float dx, float dy, sol::optional<float> duration) {
        log::info("Lua Object.move({}, {}, {}, {})",
            groupID, dx, dy, duration.value_or(0.f)
        );

        float dur = duration.value_or(0.0f);

        moveGroupWithEasing(layer, groupID, { dx, dy }, dur);
    };

    groupTable["rotate"] = [layer](
        int targetGroupID, 
        int centerGroupID, 
        int degrees, 
        sol::optional<int> times360, 
        sol::optional<float> duration,
        sol::optional<int> easingType,
        sol::optional<float> easingRate
    ) {
        log::info("Lua Object.rotate({}, {}, {}, {})", 
            targetGroupID, centerGroupID, degrees, duration.value_or(0.f)
        );

        rotateGroupWithEasing(
            layer, 
            targetGroupID, 
            centerGroupID, 
            degrees, 
            times360.value_or(0), 
            duration.value_or(0.0f),
            easingType.value_or(0),
            easingRate.value_or(2.0f)
        );
    };

    groupTable["scale"] = [layer](
        int targetGroupID, 
        int centerGroupID, 
        float scaleX, 
        sol::optional<float> scaleY, 
        sol::optional<float> duration,
        sol::optional<int> easingType,
        sol::optional<float> easingRate,
        sol::optional<sol::table> options
    ) {
        log::info("Lua Group.scale({}, {}, {}, {})", 
            targetGroupID, centerGroupID, scaleX, duration.value_or(0.f)
        );

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

        scaleGroupWithEasing(
            layer, 
            targetGroupID, 
            centerGroupID, 
            scaleX, 
            sy, 
            dur,
            easingType.value_or(0),
            easingRate.value_or(2.0f),
            divByX,
            divByY,
            onlyMove,
            relativeScale,
            relativeRotation
        );
    };
    // Not ready yet!
    /*m_lua->new_usertype<LuaObject>(
        "LuaObject",

        "addToLayer",
        &LuaObject::addToLayer,

        "setPosition",
        &LuaObject::setPosition,

        "getPosition",
        &LuaObject::getPosition,

        "move",
        &LuaObject::move,


        "setRotation",
        &LuaObject::setRotation,

        "getRotation",
        &LuaObject::getRotation,

        "rotate",
        &LuaObject::rotate,


        "setScale",
        &LuaObject::setScale,

        "getScale",
        &LuaObject::getScale,

        "scale",
        &LuaObject::scale,


        "getID",
        &LuaObject::getID
    );
    groupTable["new"] = [layer](int id) -> LuaObject* {
        auto obj = new LuaObject(layer,id);
        obj->addToLayer();
        return obj;
    };*/

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
        m_persistentState = m_lua->create_table();
        m_lua->globals()["state"] = m_persistentState;
    });
}

sol::environment LuaManager::createScriptEnvironment() {
    sol::environment env(*m_lua, sol::create);

    env["state"] = m_persistentState;

    auto meta = m_lua->create_table();
    meta["__index"] = m_lua->globals();

    env[sol::metatable_key] = meta;

    return env;
}

void LuaManager::executeScript(const std::string& code) {
    if (!m_lua) return;

    sol::environment env = createScriptEnvironment();

    auto loaded = m_lua->load(
        "local _ENV = ...\n" + code
    );

    if (!loaded.valid()) {
        sol::error err = loaded;
        log::error("Lua Load Error: {}", err.what());
        return;
    }

    sol::coroutine thread(loaded);

    auto result = thread(env);

    if (!result.valid()) {
        sol::error err = result;
        log::error("Lua Runtime Error: {}", err.what());
        return;
    }

    runCoroutine(thread);
}

void LuaManager::runCoroutine(sol::coroutine coroutine) {
    auto result = coroutine();

    if (!result.valid()) {
        sol::error err = result;
        log::error("Lua Coroutine Error: {}", err.what());
        return;
    }

    if (!coroutine.runnable()) return;

    if (result.return_count() > 0) {
        float seconds = result.get<float>(0);

        auto scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;

        scene->runAction(
            cocos2d::CCSequence::create(
                cocos2d::CCDelayTime::create(seconds),
                CallFuncExt::create([this, coroutine]() mutable {
                    this->runCoroutine(coroutine);
                }),
                nullptr
            )
        );
    }
}
