#include <DebugConsole.hpp>

DebugConsole* DebugConsole::instance = nullptr;

DebugConsole* DebugConsole::get() {
    return instance;
}

static const std::string HEADER_TEXT = "Lua Trigger Debug Console\nPress Ctrl+K to clear the console.";

bool DebugConsole::init() {
    if (!CCLayer::init()) return false;

    instance = this;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    this->setContentSize({ winSize.width * 0.8f, winSize.height * 0.9f });
    this->setPosition({ winSize.width * 0.1f, winSize.height * 0.05f });

    auto bg = CCLayerColor::create();
    bg->setColor({ 50, 50, 50 });
    bg->setOpacity(175);
    bg->setContentSize(this->getContentSize());
    this->addChild(bg);

    std::string finalText = HEADER_TEXT;
    const auto& logs = LogManager::get().getLogs();
    for (const auto& log : logs) {
        finalText += "\n" + log.colorFormatted;
    }

    m_textArea = amber::ScrollTextArea::create(
        finalText,
        this->getContentSize(),
        0.75f,
        "jetbrains.fnt"_spr,
        { 0, 0, 0, 0 }
    );
    m_textArea->setAnchorPoint({ 0.f, 0.f });
    this->addChild(m_textArea);

    return true;
}

void DebugConsole::addLogEntry(const LogEntry& entry) {
    if (!m_textArea) return;

    std::string currentText = std::string(m_textArea->getText());
    if (!currentText.empty()) {
        currentText += "\n";
    }
    currentText += entry.colorFormatted;

    m_textArea->setText(currentText, true);
}

void DebugConsole::updateContent() {
    if (!m_textArea) return;

    std::string finalText = HEADER_TEXT;
    const auto& logs = LogManager::get().getLogs();
    for (const auto& log : logs) {
        finalText += "\n" + log.colorFormatted;
    }

    m_textArea->setText(finalText, true);
}

void DebugConsole::onExit() {
    if (instance == this) {
        instance = nullptr;
    }
    CCLayer::onExit();
}

DebugConsole* DebugConsole::create() {
    if (instance) {
        return instance;
    }

    auto ret = new DebugConsole();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}