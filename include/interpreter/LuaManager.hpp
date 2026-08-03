#pragma once

#include <Geode/Geode.hpp>
#include <miskaa.notif/src/includes/notif_api.hpp>
#include <sol/sol.hpp>
#include <interpreter/Utils.hpp>

class LuaManager {
public:
    LuaManager();
    std::string highlightSyntax(const std::string& code);
    void executeScript(const std::string& code);
    void runCoroutine(std::shared_ptr<sol::coroutine> coroutine, sol::environment env);
private:
    std::unique_ptr<sol::state> m_lua;
    sol::table m_persistentState;

    sol::environment createScriptEnvironment();
};
