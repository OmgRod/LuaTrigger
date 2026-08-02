#pragma once

#include <Geode/Geode.hpp>
#include <smjs.object-collab/include/object_collab.hpp>
#include "LuaManager.hpp"

using namespace geode::prelude;

class LuaTrigger : public object_collab::CustomObject<SpawnTriggerGameObject> {
    static constexpr uint32_t SCRIPT   = 140;
    static constexpr uint32_t FILENAME = 141;
public:
    static LuaTrigger* create();
    static object_collab::PopupOptions getEditObjectConfig(const object_collab::Selected& selected);
private:
    std::string m_b64code;
    std::string m_filename;
    bool m_active;
    LuaManager m_luaManager;
public:
    LuaTrigger();
    void postInit() override;
    void triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) override;
    bool ignoreEditorDuration() override;
    void checkMod();
};
