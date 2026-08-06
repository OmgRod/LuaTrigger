#pragma once

#include <Geode/Geode.hpp>
#include <smjs.object-collab/include/object_collab.hpp>
#include <utils/Utils.hpp>
#include <utils/Events.hpp>
#include <LuaInterpreter.hpp>

using namespace geode::prelude;
using namespace object_collab::prelude;

class ConditionLuaTrigger : public object_collab::CustomObject<SpawnTriggerGameObject> {
public:
    // Property IDs — must not overlap with other triggers.
    // 140 = Lua condition expression (base64)
    // 142 = true group ID
    // 143 = false group ID
    static constexpr uint32_t SCRIPT      = 140;
    static constexpr uint32_t TRUE_GROUP  = 142;
    static constexpr uint32_t FALSE_GROUP = 143;

    static ConditionLuaTrigger* create(ObjectInfo* info);
    static object_collab::PopupOptions getEditObjectConfig(const object_collab::Selected& selected);

    ConditionLuaTrigger(ObjectInfo* info);

    void postInit() override;
    void triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) override;

    bool ignoreEditorDuration() override;

    std::string m_b64code;
    int m_trueGroup  = 0;
    int m_falseGroup = 0;

    void setupResetListener();
    void resetLuaState();

    ListenerHandle m_resetListener;
};
