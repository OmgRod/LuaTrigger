#include "LuaManager.hpp"
#include <Geode/Geode.hpp>
#include <cctype>
#include <Geode/utils/async.hpp>
#include <asp/time/sleep.hpp>
#include <asp/time/Duration.hpp>

using namespace geode::prelude;

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

    moveTrigger->triggerObject(gameLayer, -1, nullptr);
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
        log::info("Lua Group.move({}, {}, {}, {})",
            groupID, dx, dy, duration.value_or(0.f)
        );

        GJBaseGameLayer* layer = nullptr;
        if (auto playLayer = PlayLayer::get()) layer = playLayer;
        else if (auto editorLayer = LevelEditorLayer::get()) layer = editorLayer;
        if (!layer) return;

        float dur = duration.value_or(0.0f);

        moveGroupWithEasing(layer, groupID, { dx, dy }, dur);
    };

    m_lua->set_function("wait", [](double seconds, sol::this_state s) {
        lua_pushnumber(s, seconds);
        return lua_yield(s, 1);
    });
}

void LuaManager::executeScript(const std::string& code) {
    if (!m_lua) return;

    auto loaded = m_lua->load(code);

    if (!loaded.valid()) {
        sol::error err = loaded;
        log::error("Lua Load Error: {}", err.what());
        return;
    }

    sol::coroutine coroutine(loaded);

    runCoroutine(coroutine);
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

