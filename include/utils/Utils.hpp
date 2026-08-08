#pragma once

#include <Geode/Geode.hpp>
#include <sol/sol.hpp>

using namespace geode::prelude;

void moveGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, CCPoint offset, float duration, int easingType = 0, float easingRate = 2.0f);
void rotateGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, int degrees, int times360, float duration, int easingType = 0, float easingRate = 2.0f, bool lockObjRotation = false);
void scaleGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, float scaleX, float scaleY, float duration, int easingType = 0, float easingRate = 2.0f, bool divByX = false, bool divByY = false, bool onlyMove = false, bool relativeScale = false, bool relativeRotation = false);
void togglePlayerMovement(GJBaseGameLayer* layer, bool enabled);

std::string formatLuaArgs(sol::variadic_args args);
std::pair<std::string, bool> processLuaLogArgs(sol::variadic_args args);
bool isKeyword(const std::string& word);
bool isFunction(const std::string& word, const std::string& code, size_t pos);
bool isClass(const std::string& word);

class DialogCleanupNode : public CCNode {
public:
    GJBaseGameLayer* m_layer;

    static DialogCleanupNode* create(GJBaseGameLayer* layer) {
        auto ret = new DialogCleanupNode();
        if (ret && ret->init()) {
            ret->m_layer = layer;
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    ~DialogCleanupNode() override {
        if (m_layer) {
            togglePlayerMovement(m_layer, false);
        }
    }
};