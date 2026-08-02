#pragma once
#include <sol/sol.hpp>
#include <memory>
#include <string>

class LuaManager {
public:
    LuaManager();
    void executeScript(const std::string& code);
private:
    std::unique_ptr<sol::state> m_lua;
};
