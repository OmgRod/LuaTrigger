#pragma once

#include <Geode/Geode.hpp>
#include <smjs.object-collab/include/object_collab.hpp>
#include <nodes/FileSelectNode.hpp>
#include <nodes/FileValueMenu.hpp>
#include <fryy_55.amber/include/amber.hpp>
#include <miskaa.notif/src/includes/notif_api.hpp>
#include <sol/sol.hpp>
#include <utils/Utils.hpp>
#include <utils/Events.hpp>

using namespace geode::prelude;
using namespace object_collab::prelude;

class LuaTrigger : public object_collab::CustomObject<EffectGameObject> {
public:
    static constexpr uint32_t SCRIPT   = 140;
    static constexpr uint32_t FILENAME = 141;

    static LuaTrigger* create(ObjectInfo* info);
    static object_collab::PopupOptions getEditObjectConfig(const object_collab::Selected& selected);

    LuaTrigger(ObjectInfo* info);

    void postInit() override;
    void triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) override;

    bool ignoreEditorDuration() override;
    void checkMod();

    void stopLua();
    void pauseLua();
    void resumeLua();

    static std::string highlightSyntax(const std::string& code);
    void executeScript(const std::string& code);
    inline void runCoroutine(std::shared_ptr<sol::coroutine> coroutine, sol::environment env, uint32_t token);
    
    std::string m_b64code;
    std::string m_filename;
    
    bool m_active = false;
    bool m_disabled = false;
    
    std::unique_ptr<sol::state> m_lua;
    sol::table m_persistentState;

    sol::environment createScriptEnvironment();

    inline void setupLuaInterpreter();

    void setupResetListener();
    void resetLuaState();

    ListenerHandle m_resetListener;
    uint32_t m_executionToken = 0;
};
