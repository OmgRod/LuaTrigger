#pragma once

#include <sol/sol.hpp>
#include <Geode/Geode.hpp>
#include <unordered_map>
#include <memory>

class LuaInterpreter {
public:
    LuaInterpreter();
    ~LuaInterpreter() = default;

    /// Returns the shared LuaInterpreter for the given GJBaseGameLayer.
    /// All triggers on the same layer share one interpreter (and therefore
    /// one persistent `state` table) through this registry.
    static std::shared_ptr<LuaInterpreter> forLayer(GJBaseGameLayer* layer);

    /// Initialises (or reuses) the shared Lua state for the given layer.
    /// Safe to call multiple times — subsequent calls are no-ops.
    void init(GJBaseGameLayer* layer);

    bool runString(const std::string& code);

    template <typename T>
    std::optional<T> evaluateExpression(const std::string& expression);

    sol::environment createScriptEnvironment();
    void runCoroutine(std::shared_ptr<sol::coroutine> coroutine, sol::environment env, uint32_t token);

    void stop();
    void pause();
    void resume();
    void resetState();

    sol::state& getState() { return *m_lua; }

    /// Call when a GJBaseGameLayer is being destroyed to free its shared Lua state.
    static void cleanupLayer(GJBaseGameLayer* layer);

private:
    void bindEngineAPI(GJBaseGameLayer* layer);
    static std::string formatLuaArgs(sol::variadic_args args);

    // The actual Lua VM — shared across all triggers on the same layer.
    std::shared_ptr<sol::state> m_lua;
    sol::table m_persistentState;

    GJBaseGameLayer* m_layer = nullptr;
    bool m_initialized = false;

    // Per-interpreter (but now per-layer): controls coroutine execution.
    bool m_disabled = false;
    uint32_t m_executionToken = 0;

    // Global registry: one LuaInterpreter per active GJBaseGameLayer.
    static std::unordered_map<GJBaseGameLayer*, std::shared_ptr<LuaInterpreter>> s_registry;
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