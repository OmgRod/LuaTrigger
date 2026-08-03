#include <utils/Utils.hpp>

void moveGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, cocos2d::CCPoint offset, float duration, int easingType, float easingRate) {
    if (!gameLayer) return;

    auto moveTrigger = static_cast<EffectGameObject*>(GameObject::createWithKey(901));
    if (!moveTrigger) {
        log::error("Move trigger cast failed");
        return;
    }

    moveTrigger->m_targetGroupID = targetGroupID;
    moveTrigger->m_moveOffset = offset;
    moveTrigger->m_duration = duration;
    moveTrigger->m_easingType = static_cast<EasingType>(easingType);
    moveTrigger->m_easingRate = easingRate;

    moveTrigger->triggerObject(gameLayer, -1, nullptr);
}

void rotateGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, int degrees, int times360, float duration, int easingType, float easingRate, bool lockObjRotation) {
    if (!gameLayer) return;

    auto rotateTrigger = static_cast<EffectGameObject*>(GameObject::createWithKey(1346));
    if (!rotateTrigger) {
        log::error("Rotate trigger creation failed");
        return;
    }

    rotateTrigger->m_targetGroupID = targetGroupID;
    rotateTrigger->m_centerGroupID = centerGroupID;
    rotateTrigger->m_rotationDegrees = degrees;
    rotateTrigger->m_times360 = times360;
    rotateTrigger->m_duration = duration;
    rotateTrigger->m_easingType = static_cast<EasingType>(easingType);
    rotateTrigger->m_easingRate = easingRate;
    rotateTrigger->m_lockObjectRotation = lockObjRotation;

    rotateTrigger->triggerObject(gameLayer, -1, nullptr);
}

void scaleGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, float scaleX, float scaleY, float duration, int easingType, float easingRate, bool divByX, bool divByY, bool onlyMove, bool relativeScale, bool relativeRotation) {
    if (!gameLayer) return;

    auto scaleTrigger = static_cast<TransformTriggerGameObject*>(GameObject::createWithKey(2067));
    if (!scaleTrigger) {
        log::error("Scale trigger creation failed");
        return;
    }

    scaleTrigger->m_targetGroupID = targetGroupID;
    scaleTrigger->m_centerGroupID = centerGroupID;

    scaleTrigger->m_objectScaleX = scaleX;
    scaleTrigger->m_objectScaleY = scaleY;
    scaleTrigger->m_duration = duration;

    scaleTrigger->m_easingType = static_cast<EasingType>(easingType);
    scaleTrigger->m_easingRate = easingRate;

    scaleTrigger->m_divideX = divByX;
    scaleTrigger->m_divideY = divByY;
    scaleTrigger->m_onlyMove = onlyMove;
    scaleTrigger->m_relativeScale = relativeScale;
    scaleTrigger->m_relativeRotation = relativeRotation;

    scaleTrigger->triggerObject(gameLayer, -1, nullptr);
}

void togglePlayerMovement(GJBaseGameLayer* layer, bool enabled) {
    if (!layer) return;

    auto trigger = static_cast<PlayerControlGameObject*>(GameObject::createWithKey(1932));
    if (!trigger) {
        log::error("Scale trigger creation failed");
        return;
    }

    if (enabled) {
        trigger->m_targetPlayer1 = true;
        trigger->m_targetPlayer2 = true;
        trigger->m_stopJump = true;
        trigger->m_stopMove = true;
        trigger->m_stopRotation = true;
    } else {
        trigger->m_targetPlayer1 = true;
        trigger->m_targetPlayer2 = true;
        trigger->m_stopJump = false;
        trigger->m_stopMove = false;
        trigger->m_stopRotation = false;
    }

    trigger->triggerObject(layer, -1, nullptr);
}

std::string formatLuaArgs(sol::variadic_args args) {
    std::string result;
    for (auto arg : args) {
        switch (arg.get_type()) {
            case sol::type::string:
                result += arg.as<std::string>();
                break;
            case sol::type::number:
                result += std::to_string(arg.as<double>());
                break;
            case sol::type::boolean:
                result += arg.as<bool>() ? "true" : "false";
                break;
            case sol::type::lua_nil:
                result += "nil";
                break;
            default:
                result += "<" + std::string(sol::type_name(args.lua_state(), arg.get_type())) + ">";
                break;
        }
        result += "\t";
    }
    return result;
}

bool isKeyword(const std::string& word) {
    static const std::unordered_set<std::string> keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function", 
        "if", "in", "local", "nil", "not", "or", "repeat", "return", "then", 
        "true", "until", "while"
    };
    return keywords.find(word) != keywords.end();
}

bool isFunction(const std::string& word, const std::string& code, size_t pos) {
    while(pos < code.size() && std::isspace(code[pos])) pos++;
    return pos < code.size() && code[pos] == '(';
}

bool isClass(const std::string& word) {
    return !word.empty() && std::isupper(word[0]);
}