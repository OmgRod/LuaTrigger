#pragma once

#include <interpreter/Utils.hpp>

class LuaObject {
public:
    GJBaseGameLayer* layer = nullptr;
    GameObject* object = nullptr;
    int objectID = 0;
    float x = 0;
    float y = 0;
    float rotation = 0;
    float scaleX = 1;
    float scaleY = 1;

    LuaObject(GJBaseGameLayer* layer, int objectID);

    void addToLayer();
    void setPosition(float x, float y);
    std::pair<float, float> getPosition();
    void move(float x, float y, float duration = 0);

    void setRotation(float rotation);
    float getRotation();
    void rotate(float rotation, float duration = 0);

    void setScale(float x, float y);
    std::pair<float, float> getScale();
    void scale(float x, float y, float duration = 0);

    int getID();
};