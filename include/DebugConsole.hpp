#pragma once

#include <Geode/Geode.hpp>
#include <fryy_55.amber/include/classes/ScrollTextArea.hpp>
#include <utils/LogManager.hpp>

using namespace geode::prelude;

class DebugConsole : public CCLayer {

private:
    amber::ScrollTextArea* m_textArea;
    
    bool init() override;
    
public:
    static DebugConsole* instance;

    static DebugConsole* get();
    static DebugConsole* create();

    void addLogEntry(const LogEntry& entry);
    void updateContent();

    void onExit() override;
};
