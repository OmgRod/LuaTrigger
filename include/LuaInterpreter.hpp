#pragma once

#include <sol/sol.hpp>
#include <Geode/Geode.hpp>
#include <utils/LogManager.hpp>

using namespace geode::prelude;

class LuaInterpreter {
public:
    LuaInterpreter();
    ~LuaInterpreter() = default;

    static std::shared_ptr<LuaInterpreter> forLayer(GJBaseGameLayer* layer);

    void init(GJBaseGameLayer* layer);

    bool runString(const std::string& code, bool ignoreTimeout = false);

    template <typename T>
    std::optional<T> evaluateExpression(const std::string& expression, bool ignoreTimeout = false);

    sol::environment createScriptEnvironment();
    void runCoroutine(std::shared_ptr<sol::coroutine> coroutine, sol::environment env, uint32_t token, bool ignoreTimeout = false);

    void stop();
    void pause();
    void resume();
    void resetState();

    sol::state& getState() { return *m_lua; }

    static void cleanupLayer(GJBaseGameLayer* layer);

private:
    void bindEngineAPI(GJBaseGameLayer* layer);
    static std::string formatLuaArgs(sol::variadic_args args);

    std::shared_ptr<sol::state> m_lua;
    sol::table m_persistentState;

    GJBaseGameLayer* m_layer;
    bool m_initialized = false;

    bool m_disabled = false;
    uint32_t m_executionToken = 0;

    static std::unordered_map<GJBaseGameLayer*, std::shared_ptr<LuaInterpreter>> s_registry;
};

// welcome to sol
template <typename T>
std::optional<T> LuaInterpreter::evaluateExpression(const std::string& expression, bool ignoreTimeout) {
    if (!m_lua) return std::nullopt;

    lua_State* L = m_lua->lua_state();
    if (L && !ignoreTimeout) {
        lua_sethook(L, [](lua_State* L, lua_Debug*) {
            int* count = static_cast<int*>(lua_getextraspace(L));
            if (count) {
                *count += 1000;
                if (*count > 100000) {
                    luaL_error(L, "Script execution limit exceeded (possible infinite loop)");
                }
            }
        }, LUA_MASKCOUNT, 1000);
        int* countPtr = static_cast<int*>(lua_getextraspace(L));
        if (countPtr) *countPtr = 0;
    }

    auto result = m_lua->script("return (" + expression + ")", [](lua_State*, sol::protected_function_result pfr) {
        return pfr;
    });

    if (L && !ignoreTimeout) {
        lua_sethook(L, nullptr, 0, 0);
    }

    if (result.valid()) {
        return result.template get<T>();
    } else {
        sol::error err = result;
        std::string errMsg = fmt::format("Lua Eval Error: {}", err.what());
        log::error("{}", errMsg);
        LogManager::get().error(errMsg);
    }

    return std::nullopt;
}