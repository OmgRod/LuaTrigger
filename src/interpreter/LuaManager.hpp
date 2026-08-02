#pragma once
#include <Geode/Geode.hpp>
#include <miskaa.notif/src/includes/notif_api.hpp>
#include <sol/sol.hpp>

class LuaManager {
public:
    LuaManager();
    std::string highlightSyntax(const std::string& code);
    void executeScript(const std::string& code);
    void runCoroutine(sol::coroutine coroutine);
private:
    std::unique_ptr<sol::state> m_lua;
    sol::table m_persistentState;

    sol::environment createScriptEnvironment();
};
