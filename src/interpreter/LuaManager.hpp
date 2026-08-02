#pragma once
#include <sol/sol.hpp>
#include <memory>
#include <string>

class LuaManager {
public:
    LuaManager();
    std::string highlightSyntax(const std::string& code);
    void executeScript(const std::string& code);
    void runCoroutine(sol::coroutine coroutine);
private:
    std::unique_ptr<sol::state> m_lua;
};
