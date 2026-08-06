#include <ExecuteLuaTrigger.hpp>
#include <nodes/ExamplesPopup.hpp>

bool ExamplesPopup::init(ExecuteLuaTrigger* trigger, std::function<void(const std::string&)> onCodeSelected) {
    if (!Popup::init(300.f, 220.f)) return false;

    this->m_trigger = trigger;
    this->setTitle("Example Scripts");

    auto res = geode::utils::file::readJson(Mod::get()->getResourcesDir() / "examples" / "meta.json");
    if (!res) {
        log::error("Failed to read examples meta.json: {}", res.unwrapErr());
        return false;
    }

    matjson::Value root = res.unwrap();

    auto bg = CCScale9Sprite::create("square02b_001.png", { 0.f, 0.f, 80.f, 80.f });
    bg->setContentSize({ 260.f, 150.f });
    bg->setColor({ 0, 0, 0 });
    bg->setOpacity(100);
    bg->setPosition({ 150.f, 95.f });
    this->m_mainLayer->addChild(bg);

    auto list = ScrollLayer::create({ 260.f, 150.f }, true, true);
    list->setPosition({ 20.f, 20.f });
    
    auto contents = list->m_contentLayer;

    auto layout = AxisLayout::create(Axis::Column)
        ->setGap(8.f)
        ->setAxisReverse(true)
        ->setCrossAxisOverflow(false)
        ->setAutoScale(false);

    contents->setLayout(layout);

    if (root.contains("examples") && root["examples"].isArray()) {
        auto examples = root["examples"].asArray().unwrap();

        for (auto const& example : examples) {
            if (!example.isObject()) continue;

            std::string name = example["name"].asString().unwrapOr("Unnamed Example");
            std::string desc = example["description"].asString().unwrapOr("No description provided.");
            std::string filename = example["path"].asString().unwrapOr("");
            bool locked = example["locked"].asBool().unwrapOr(false);

            auto script = new ExampleScript();
            script->name = name;
            script->description = desc;
            script->filename = filename;
            script->locked = locked;

            auto cell = ExampleCell::create(trigger, this, script, onCodeSelected);
            contents->addChild(cell);
        }
    }

    float totalHeight = 0.f;
    for (auto child : CCArrayExt<CCNode*>(contents->getChildren())) {
        totalHeight += child->getContentHeight() + layout->getGap();
    }
    
    float minHeight = 150.f; 
    contents->setContentSize({ 260.f, std::max(minHeight, totalHeight) });

    contents->updateLayout();
    
    list->scrollToTop();

    this->m_mainLayer->addChild(list);

    return true;
}

ExamplesPopup* ExamplesPopup::create(ExecuteLuaTrigger* trigger, std::function<void(const std::string&)> onCodeSelected) {
    auto ret = new ExamplesPopup();

    if (ret && ret->init(trigger, onCodeSelected)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool ExampleCell::init(ExecuteLuaTrigger* trigger, ExamplesPopup* popup, ExampleScript* script, std::function<void(const std::string&)> onCodeSelected) {
    if (!CCNode::init()) return false;

    auto bg = CCScale9Sprite::create("GJ_square01.png", { 0.f, 0.f, 80.f, 80.f });
    bg->setContentSize({ 250.f, 48.f });
    bg->setAnchorPoint({ 0.f, 0.f });
    bg->setPosition({ 0.f, 0.f });
    this->addChild(bg);

    CCSize size = bg->getContentSize();
    this->setContentSize(size);

    auto nameLabel = CCLabelBMFont::create(script->name.c_str(), "goldFont.fnt");
    nameLabel->setAnchorPoint({ 0.f, 0.5f });
    nameLabel->setPosition({ 10.f, size.height - 15.f });
    nameLabel->setScale(0.45f);
    this->addChild(nameLabel);

    auto descLabel = CCLabelBMFont::create(script->description.c_str(), "chatFont.fnt");
    descLabel->setAnchorPoint({ 0.f, 0.5f });
    descLabel->setPosition({ 10.f, 14.f });
    descLabel->setScale(0.45f);
    descLabel->setColor({ 200, 200, 200 });
    
    if (descLabel->getContentWidth() * 0.45f > (size.width - 70.f)) {
        descLabel->setScale((size.width - 70.f) / descLabel->getContentWidth());
    }
    this->addChild(descLabel);

    ButtonSprite* btnSpr = nullptr;
    if (script->locked) {
        btnSpr = ButtonSprite::create("Locked", 40, true, "goldFont.fnt", "GJ_button_02.png", 20.f, 0.45f);
    } else {
        btnSpr = ButtonSprite::create("Use", 35, true, "goldFont.fnt", "GJ_button_01.png", 20.f, 0.45f);
    }

    auto selectBtn = CCMenuItemExt::createSpriteExtra(
        btnSpr,
        [trigger, popup, script, onCodeSelected](CCObject*) {
            if (script->locked) {
                FLAlertLayer::create(
                    "Example Locked",
                    "This example is currently unavailable.",
                    "OK"
                )->show();
                return;
            }

            auto res = geode::utils::file::readString(Mod::get()->getResourcesDir() / "examples" / script->filename);
            if (!res) {
                FLAlertLayer::create(
                    "Error",
                    fmt::format("Failed to read example file: {}", res.unwrapErr()),
                    "OK"
                )->show();
                return;
            }

            std::string code = res.unwrap();

            gd::string encoded = LevelTools::base64EncodeString(gd::string(code.c_str(), code.size()));

            trigger->m_b64code = std::string(encoded.c_str(), encoded.size());
            trigger->m_filename = script->filename;
            trigger->checkMod();

            if (onCodeSelected) {
                onCodeSelected(code);
            }

            if (popup) {
                popup->closePopup();
            }
        }
    );

    auto menu = CCMenu::create();
    menu->setContentSize(size);
    menu->setPosition({ 0.f, 0.f });

    selectBtn->setPosition({ size.width - 28.f, size.height / 2.f });
    menu->addChild(selectBtn);

    this->addChild(menu);

    return true;
}

ExampleCell* ExampleCell::create(ExecuteLuaTrigger* trigger, ExamplesPopup* popup, ExampleScript* script, std::function<void(const std::string&)> onCodeSelected) {
    auto ret = new ExampleCell();

    if (ret && ret->init(trigger, popup, script, onCodeSelected)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}