#pragma once

#include <sol/sol.hpp>
#include <Geode/Geode.hpp>

class LuaInterpreter {
public:
    LuaInterpreter();
    ~LuaInterpreter() = default;

    void init(GJBaseGameLayer* layer);

    bool runString(const std::string& code);

    template <typename T>
    std::optional<T> evaluateExpression(const std::string& expression);

    void resetState();

    sol::state& getState() { return *m_lua; }

private:
    void bindEngineAPI(GJBaseGameLayer* layer);
    static std::string formatLuaArgs(sol::variadic_args args);

    std::unique_ptr<sol::state> m_lua;
    sol::table m_persistentState;
    bool m_initialized = false;
};

template <typename T>
std::optional<T> LuaInterpreter::evaluateExpression(const std::string& expression) {
    if (!m_lua) return std::nullopt;

    auto result = m_lua->script("return (" + expression + ")", [](lua_State*, sol::protected_function_result pfr) {
        return pfr;
    });

    if (result.valid()) {
        return result.template get<T>();
    } else {
        sol::error err = result;
        geode::log::error("[Lua Eval Error] {}", err.what());
    }

    return std::nullopt;
}