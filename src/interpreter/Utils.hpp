#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

void moveGroupWithEasing(
    GJBaseGameLayer* gameLayer,
    int targetGroupID,
    cocos2d::CCPoint offset,
    float duration,
    int easingType = 0,
    float easingRate = 2.0f
);

void rotateGroupWithEasing(
    GJBaseGameLayer* gameLayer,
    int targetGroupID,
    int centerGroupID,
    int degrees,
    int times360,
    float duration,
    int easingType = 0,
    float easingRate = 2.0f,
    bool lockObjRotation = false
);

void scaleGroupWithEasing(
    GJBaseGameLayer* gameLayer,
    int targetGroupID,
    int centerGroupID,
    float scaleX,
    float scaleY,
    float duration,
    int easingType = 0,
    float easingRate = 2.0f,
    bool divByX = false,
    bool divByY = false,
    bool onlyMove = false,
    bool relativeScale = false,
    bool relativeRotation = false
);
