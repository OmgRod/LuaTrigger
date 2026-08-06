#pragma once

#include <Geode/Geode.hpp>
#include <smjs.object-collab/include/object_collab.hpp>
#include <nodes/FileSelectNode.hpp>
#include <nodes/FileValueMenu.hpp>
#include <fryy_55.amber/include/amber.hpp>
#include <miskaa.notif/src/includes/notif_api.hpp>
#include <utils/Utils.hpp>
#include <utils/Events.hpp>
#include <LuaInterpreter.hpp>

using namespace geode::prelude;
using namespace object_collab::prelude;

class ExecuteLuaTrigger : public object_collab::CustomObject<EffectGameObject> {
public:
    static constexpr uint32_t SCRIPT   = 140;
    static constexpr uint32_t FILENAME = 141;

    static ExecuteLuaTrigger* create(ObjectInfo* info);
    static object_collab::PopupOptions getEditObjectConfig(const object_collab::Selected& selected);

    ExecuteLuaTrigger(ObjectInfo* info);

    void postInit() override;
    void triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) override;

    void checkMod();

    void stopLua();
    void pauseLua();
    void resumeLua();

    static std::string highlightSyntax(const std::string& code);

    std::string m_b64code;
    std::string m_filename;

    bool m_active = false;

    void setupResetListener();
    void resetLuaState();

    ListenerHandle m_resetListener;
};
