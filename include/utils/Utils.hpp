#pragma once

#include <Geode/Geode.hpp>
#include <sol/sol.hpp>

using namespace geode::prelude;

void moveGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, cocos2d::CCPoint offset, float duration, int easingType = 0, float easingRate = 2.0f);
void rotateGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, int degrees, int times360, float duration, int easingType = 0, float easingRate = 2.0f, bool lockObjRotation = false);
void scaleGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, float scaleX, float scaleY, float duration, int easingType = 0, float easingRate = 2.0f, bool divByX = false, bool divByY = false, bool onlyMove = false, bool relativeScale = false, bool relativeRotation = false);
void togglePlayerMovement(GJBaseGameLayer* layer, bool enabled);

std::string formatLuaArgs(sol::variadic_args args);
bool isKeyword(const std::string& word);
bool isFunction(const std::string& word, const std::string& code, size_t pos);
bool isClass(const std::string& word);