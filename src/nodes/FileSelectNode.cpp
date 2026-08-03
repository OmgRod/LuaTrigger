#include <nodes/FileSelectNode.hpp>

bool FileSelectNode::init(float width) {
    if (!CCNode::init()) return false;

    this->setContentSize({ width, 54.0f });

    auto background = CCScale9Sprite::create("square02b_001.png");
    background->setContentSize({ width, 27.0f });
    background->setColor({ 0, 0, 0 });
    background->setOpacity(100);
    background->setAnchorPoint({ 0.f, 0.f });
    background->setPosition({ 0.f, 0.f });
    this->addChild(background);

    m_nameLabel = CCLabelBMFont::create("No file selected", "chatFont.fnt");
    m_nameLabel->setAnchorPoint({ 0.f, 0.5f });
    m_nameLabel->setPosition({ 5.f, 13.5f });
    m_nameLabel->setColor(ccGRAY);
    m_nameLabel->setOpacity(180);
    this->addChild(m_nameLabel);

    auto btnSpr = ButtonSprite::create("Choose File", "goldFont.fnt", "GJ_button_01.png");
    btnSpr->setScale(0.6f);

    m_selectBtnSpr = CCSprite::createWithSpriteFrameName("GJ_plus2Btn_001.png");
    m_selectBtn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(FileSelectNode::onPickFile));

    auto buttonMenu = CCMenu::create();
    buttonMenu->setPosition({ 0.f, 0.f });
    buttonMenu->setContentSize({ width, 54.0f });
    buttonMenu->addChild(m_selectBtn);

    m_selectBtn->setPosition({ width - 57.f, 13.5f });
    this->addChild(buttonMenu);

    this->updateState();

    return true;
}

void FileSelectNode::updateState() {
    float labelMaxWidth = this->getContentSize().width - 115.f;

    if (m_path.empty()) {
        m_nameLabel->setString("No file selected");
        m_nameLabel->setColor(ccGRAY);
        m_nameLabel->setOpacity(180);
    } else {
        m_nameLabel->setString(geode::utils::string::pathToString(m_path.filename()).c_str());
        m_nameLabel->setColor(ccWHITE);
        m_nameLabel->setOpacity(255);
    }
    m_nameLabel->limitLabelWidth(labelMaxWidth, 0.55f, 0.1f);
}

void FileSelectNode::onPickFile(CCObject*) {
    auto defaultPath = m_path.empty() ? dirs::getGameDir() : m_path;

    m_pickListener.spawn(
        file::pick(
            file::PickMode::OpenFile,
            {
                defaultPath,
                { { "Lua Files", { "*.lua" } } }
            }
        ),
        [this](Result<std::optional<std::filesystem::path>> path) {
            if (path.isOk() && path.unwrap().has_value()) {
                auto picked = std::move(path).unwrap().value();
                this->setPath(picked);
                if (m_onFileSelected) {
                    m_onFileSelected(m_path);
                }
            } else if (path.isErr()) {
                FLAlertLayer::create(
                    "Failed",
                    fmt::format("Failed to pick file: {}", path.unwrapErr()),
                    "Ok"
                )->show();
            }
        }
    );
}

void FileSelectNode::setPath(std::filesystem::path path) {
    m_path = std::move(path);
    this->updateState();
}

void FileSelectNode::setOnFileSelected(std::function<void(std::filesystem::path const&)> callback) {
    m_onFileSelected = std::move(callback);
}

void FileSelectNode::preloadFilename(std::string const& filename) {
    float labelMaxWidth = this->getContentSize().width - 115.f;
    m_nameLabel->setString(filename.c_str());
    m_nameLabel->setColor(ccWHITE);
    m_nameLabel->setOpacity(255);
    m_nameLabel->limitLabelWidth(labelMaxWidth, 0.55f, 0.1f);
}

FileSelectNode* FileSelectNode::create(float width) {
    auto ret = new FileSelectNode();
    if (ret->init(width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
