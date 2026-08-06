#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/SpawnTriggerGameObject.hpp>
#include <smjs.object-collab/include/object_collab.hpp>
#include <utils/Utils.hpp>
#include <utils/Events.hpp>
#include <LuaInterpreter.hpp>

using namespace geode::prelude;
using namespace object_collab::prelude;

class ConditionLuaTrigger : public object_collab::CustomObject<SpawnTriggerGameObject> {
public:
    static constexpr uint32_t SCRIPT      = 140;
    static constexpr uint32_t TRUE_GROUP  = 142;
    static constexpr uint32_t FALSE_GROUP = 143;

    std::string m_b64code = "";
    int m_trueGroup = 0;
    int m_falseGroup = 0;
    
    bool m_active = false;

    static ConditionLuaTrigger* create(ObjectInfo* info);
    static object_collab::PopupOptions getEditObjectConfig(const object_collab::Selected& selected);

    ConditionLuaTrigger(ObjectInfo* info);

    void postInit() override;
    void triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) override;

    bool ignoreEditorDuration() override;

    void setupResetListener();
    void resetLuaState();

    void checkMod();

    ListenerHandle m_resetListener;
};