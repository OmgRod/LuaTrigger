#include <ConditionLuaTrigger.hpp>

ConditionLuaTrigger::ConditionLuaTrigger(ObjectInfo* info)
    : CustomObject(info, ObjectTraits::builder()
        .gameObjectType(GameObjectType::Modifier)
        .ignoreEditorDuration(true)
        .build()) {}

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
                
                for (auto* obj : selected) {
                    if (auto* t = geode::cast::typeinfo_cast<ConditionLuaTrigger*>(obj)) {
                        t->checkMod();
                    }
                }
            })
            .currentValue([selected](const Selected& sel, geode::Popup*) -> std::string {
                std::string enc = getCommonValueOrDefault(sel, &ConditionLuaTrigger::m_b64code);
                if (enc.empty()) return "";
                return std::string(LevelTools::base64DecodeString(enc).c_str());
            })
            .build())
        .menu(ToggleMenu::builder()
            .id("ignore-timeout")
            .title("Ignore Timeout")
            .onValue([selected](bool value, const Selected& sel, geode::Popup*) {
                applyValueToSelected(sel, &ConditionLuaTrigger::m_ignoreTimeout, value);
            })
            .currentValue([selected](const Selected& sel, geode::Popup*) -> bool {
                return getCommonValueOrDefault(sel, &ConditionLuaTrigger::m_ignoreTimeout);
            })
            .build())
        .triggerToggles(true)
        .build();
}

ConditionLuaTrigger::~ConditionLuaTrigger() {
    if (auto* pl = PlayLayer::get()) {
        LuaInterpreter::cleanupLayer(pl);
    } else if (auto* g = GJBaseGameLayer::get()) {
        LuaInterpreter::cleanupLayer(g);
    }
}

void ConditionLuaTrigger::postInit() {
    this->setHitbox({ 1, 1 });
    this->checkMod();
    this->setupResetListener();
}

void ConditionLuaTrigger::checkMod() {
    m_active = !m_b64code.empty();
}

void ConditionLuaTrigger::triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) {
    auto interp = LuaInterpreter::forLayer(layer);
    if (!interp) return;

    int targetGroup = m_falseGroup;

    if (!m_b64code.empty()) {
        std::string rawExpr = LevelTools::base64DecodeString(m_b64code);
        
        std::string wrappedExpr = "not not (" + rawExpr + ")";

        auto result = interp->evaluateExpression<bool>(wrappedExpr, m_ignoreTimeout);
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

$on_mod(Loaded) {
    ObjectAPI::registerObject(ObjectInfo::builder()
        .id("condition-trigger"_spr)
        .sprite("condition.png"_spr)
        .construction(ComplexObject::builder()
            .factory(ConditionLuaTrigger::create)
            .customProperties({
                PropertyInterface::from(ConditionLuaTrigger::SCRIPT,         &ConditionLuaTrigger::m_b64code,       std::string("")),
                PropertyInterface::from(ConditionLuaTrigger::TRUE_GROUP,     &ConditionLuaTrigger::m_trueGroup,     0),
                PropertyInterface::from(ConditionLuaTrigger::FALSE_GROUP,    &ConditionLuaTrigger::m_falseGroup,    0),
                PropertyInterface::from(ConditionLuaTrigger::IGNORE_TIMEOUT, &ConditionLuaTrigger::m_ignoreTimeout, false),
            })
            .build())
        .editObject(ConditionLuaTrigger::getEditObjectConfig)
        .editorTab(EditorTab::Triggers)
        .build());
}

void ConditionLuaTrigger::resetLuaState() {
    this->stopAllActions();
}

void ConditionLuaTrigger::setupResetListener() {
    m_resetListener = LevelResetEvent().listen([this]() {
        this->resetLuaState();
        return true;
    });
}