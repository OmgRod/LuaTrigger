#include <ConditionLuaTrigger.hpp>

ConditionLuaTrigger::ConditionLuaTrigger(ObjectInfo* info)
    : CustomObject(info, GameObjectType::Modifier) {}

ConditionLuaTrigger* ConditionLuaTrigger::create(ObjectInfo* info) {
    return new ConditionLuaTrigger(info);
}

PopupOptions ConditionLuaTrigger::getEditObjectConfig(const Selected& selected) {
    ConditionLuaTrigger* trig = nullptr;
    if (!selected.empty()) {
        trig = geode::cast::typeinfo_cast<ConditionLuaTrigger*>(selected[0]);
    }

    return PopupConfig::builder()
        .width(380)
        .height(220)
        .gapY(14)
        .title("Conditional Lua Trigger")
        .info(InfoPopup::builder()
            .title("Help")
            .description(
                "Evaluates a <cl>Lua</c> expression and spawns one of two groups based on the result.\n\n"
                "<cg>True Group</c> is spawned when the expression evaluates to <cg>true</c>.\n"
                "<cr>False Group</c> is spawned when the expression evaluates to <cr>false</c> or errors.\n\n"
                "The expression must return a boolean, e.g. <cy>state.score > 100</c>"
            )
            .build())
        .menu(NumericMenu::builder()
            .id("true-group")
            .title("True Group ID")
            .inputType(NumericMenu::InputType::Arrows)
            .precision(0)
            .stepSize(1)
            .min(0)
            .onValue([selected](float value, const Selected& sel, geode::Popup*) {
                applyValueToSelected(sel, &ConditionLuaTrigger::m_trueGroup, static_cast<int>(value));
            })
            .currentValue([selected](const Selected& sel, geode::Popup*) -> float {
                return static_cast<float>(getCommonValueOrDefault(sel, &ConditionLuaTrigger::m_trueGroup));
            })
            .build())
        .menu(NumericMenu::builder()
            .id("false-group")
            .title("False Group ID")
            .inputType(NumericMenu::InputType::Arrows)
            .precision(0)
            .stepSize(1)
            .min(0)
            .onValue([selected](float value, const Selected& sel, geode::Popup*) {
                applyValueToSelected(sel, &ConditionLuaTrigger::m_falseGroup, static_cast<int>(value));
            })
            .currentValue([selected](const Selected& sel, geode::Popup*) -> float {
                return static_cast<float>(getCommonValueOrDefault(sel, &ConditionLuaTrigger::m_falseGroup));
            })
            .build())
        .menu(InputMenu::builder()
            .id("condition-expr")
            .title("Condition Expression")
            .placeholder("e.g. state.score > 100")
            .allowedChars(
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "0123456789"
                " .,;:!?\"'+-*/=<>()[]{}#@%^&|~_\\"
            )
            .onValue([selected](const std::string& value, const Selected& sel, geode::Popup*) {
                gd::string encoded = LevelTools::base64EncodeString(gd::string(value.c_str(), value.size()));
                std::string enc(encoded.c_str(), encoded.size());
                applyValueToSelected(sel, &ConditionLuaTrigger::m_b64code, enc);
            })
            .currentValue([selected](const Selected& sel, geode::Popup*) -> std::string {
                std::string enc = getCommonValueOrDefault(sel, &ConditionLuaTrigger::m_b64code);
                if (enc.empty()) return "";
                return std::string(LevelTools::base64DecodeString(enc).c_str());
            })
            .build())
        .triggerToggles(true)
        .build();
}

void ConditionLuaTrigger::postInit() {
    this->setHitbox({ 1, 1 });
    this->setupResetListener();
}

void ConditionLuaTrigger::triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) {
    auto interp = LuaInterpreter::forLayer(layer);
    if (!interp) return;

    int targetGroup = m_falseGroup;

    if (!m_b64code.empty()) {
        std::string expr = LevelTools::base64DecodeString(m_b64code);
        auto result = interp->evaluateExpression<bool>(expr);

        if (result.has_value() && result.value()) {
            targetGroup = m_trueGroup;
        }
    }

    if (targetGroup > 0) {
        static const gd::vector<int> emptyRemap{};
        const gd::vector<int>& remap = remapKeys ? *remapKeys : emptyRemap;
        layer->spawnGroup(targetGroup, true, 0.0, remap, uniqueID, 0);
    }

    CustomObject::triggerObject(layer, uniqueID, remapKeys);
}

bool ConditionLuaTrigger::ignoreEditorDuration() {
    return true;
}

$on_mod(Loaded) {
    ObjectAPI::registerObject(ObjectInfo::builder()
        .id("condition-trigger"_spr)
        .sprite("condition.png"_spr)
        .construction(ComplexObject::builder()
            .factory(ConditionLuaTrigger::create)
            .customProperties({
                PropertyInterface::from(ConditionLuaTrigger::SCRIPT,      &ConditionLuaTrigger::m_b64code,   std::string("")),
                PropertyInterface::from(ConditionLuaTrigger::TRUE_GROUP,  &ConditionLuaTrigger::m_trueGroup,  0),
                PropertyInterface::from(ConditionLuaTrigger::FALSE_GROUP, &ConditionLuaTrigger::m_falseGroup, 0),
            })
            .build())
        .editObject(ConditionLuaTrigger::getEditObjectConfig)
        .editorTab(EditorTab::Triggers)
        .build());
}

void ConditionLuaTrigger::resetLuaState() {
    log::info("[ConditionLuaTrigger] Level reset - clearing interpreter state.");
    if (auto* pl = PlayLayer::get()) {
        if (auto interp = LuaInterpreter::forLayer(pl)) interp->resetState();
    }
}

void ConditionLuaTrigger::setupResetListener() {
    m_resetListener = LevelResetEvent().listen([this]() {
        this->resetLuaState();
        return true;
    });
}