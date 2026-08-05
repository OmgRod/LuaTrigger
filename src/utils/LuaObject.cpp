#include <utils/LuaObject.hpp>

LuaObject::LuaObject(GJBaseGameLayer* layer, int objectID) : layer(layer), objectID(objectID) {
    object = GameObject::createWithKey(objectID);
}

void LuaObject::addToLayer() {
    if (!layer || !object) return;

    layer->addChild(object);
}

void LuaObject::setPosition(float x, float y) {
    this->x = x;
    this->y = y;

    if (object) {
        object->setPosition({ x, y });
    }
}

std::pair<float, float> LuaObject::getPosition() {
    return {x, y};
}

void LuaObject::move(float x, float y, float duration) {
    this->x += x;
    this->y += y;

    if (!object) return;

    if (duration > 0) {
        object->runAction(
            cocos2d::CCMoveBy::create(
                duration,
                { x, y }
            )
        );
    } else {
        object->setPosition(
            object->getPosition() + cocos2d::CCPoint{x, y}
        );
    }
}

void LuaObject::setRotation(float rotation) {
    this->rotation = rotation;

    if (object) {
        object->setRotation(rotation);
    }
}

float LuaObject::getRotation() {
    return rotation;
}

void LuaObject::rotate(float rotation, float duration) {
    this->rotation += rotation;

    if (!object) return;

    if (duration > 0) {
        object->runAction(
            cocos2d::CCRotateBy::create(
                duration,
                rotation
            )
        );
    } else {
        object->setRotation(
            object->getRotation() + rotation
        );
    }
}

void LuaObject::setScale(float x, float y) {
    this->scaleX = x;
    this->scaleY = y;

    if (object) {
        object->setScaleX(x);
        object->setScaleY(y);
    }
}

std::pair<float, float> LuaObject::getScale() {
    return {scaleX, scaleY};
}

void LuaObject::scale(float x, float y,float duration) {
    scaleX *= x;
    scaleY *= y;

    if (!object) return;

    if (duration > 0) {
        object->runAction(
            cocos2d::CCScaleTo::create(
                duration,
                scaleX,
                scaleY
            )
        );
    } else {
        object->setScaleX(scaleX);
        object->setScaleY(scaleY);
    }
}

int LuaObject::getID() {
    return objectID;
}
