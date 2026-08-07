#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class LevelResetEvent : public Event<LevelResetEvent, bool()> {
public:
    using Event::Event;
};
