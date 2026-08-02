#pragma once

#include <Geode/Geode.hpp>
#include <smjs.object-collab/include/object_collab.hpp>
#include "interpreter/LuaManager.hpp"
#include "nodes/FileSelectNode.hpp"
#include "nodes/FileValueMenu.hpp"
#include "nodes/ScrollTextArea.hpp"

using namespace geode::prelude;
using namespace object_collab::prelude;

class LuaTrigger : public object_collab::CustomObject<SpawnTriggerGameObject> {
public:
    static constexpr uint32_t SCRIPT   = 140;
    static constexpr uint32_t FILENAME = 141;

    static LuaTrigger* create(ObjectInfo* info);
    static object_collab::PopupOptions getEditObjectConfig(const object_collab::Selected& selected);

    LuaTrigger(ObjectInfo* info);

    void postInit() override;
    void triggerObject(
        GJBaseGameLayer* layer,
        const int uniqueID,
        const gd::vector<int>* remapKeys
    ) override;

    bool ignoreEditorDuration() override;
    void checkMod();

    std::string m_b64code;
    std::string m_filename;

private:
    bool m_active = false;
    LuaManager m_luaManager;
};
