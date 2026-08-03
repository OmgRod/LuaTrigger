#include <LuaTrigger.hpp>

LuaTrigger::LuaTrigger(ObjectInfo* info)
    : CustomObject(info, GameObjectType::Modifier) {}

LuaTrigger* LuaTrigger::create(ObjectInfo* info) {
    return new LuaTrigger(info);
}

PopupOptions LuaTrigger::getEditObjectConfig(const Selected& selected) {
    std::string initialCode = "Upload a .lua file to see the preview here...";
    std::string initialFilename = "";

    if (!selected.empty()) {
        if (auto* trig = geode::cast::typeinfo_cast<LuaTrigger*>(selected[0])) {
            if (!trig->m_b64code.empty()) {
                initialCode = LevelTools::base64DecodeString(trig->m_b64code);
            }
            initialFilename = trig->m_filename;
        }
    }

    auto previewRef = std::make_shared<amber::ScrollTextArea*>(nullptr);

    auto luaEditor = editor_popup::CustomValueMenu::builder()
        .id("lua-preview")
        .title("Code Preview")
        .factory([initialCode, previewRef](const Selected& /*selected*/, Popup* /*popup*/) -> CCMenu* {
            auto menu = CCMenu::create();
            menu->setContentSize({ 300.0f, 150.0f });

            LuaManager luaManager;
            std::string highlightedCode = luaManager.highlightSyntax(initialCode);

            auto textPreview = amber::ScrollTextArea::create(
                highlightedCode,
                { 300.f, 150.f },
                1.f,
                "jetbrains.fnt"_spr
            );
            textPreview->setAnchorPoint({ 0.f, 0.f });
            
            *previewRef = textPreview;

            menu->addChild(textPreview);
            return menu;
        })
        .build();

    auto fileUpload = editor_popup::CustomValueMenu::builder()
        .id("file-upload")
        .title("Upload Lua File")
        .factory([selected, previewRef, initialFilename](const Selected& /*sel*/, Popup* /*popup*/) -> CCMenu* {
            auto menu = CCMenu::create();
            menu->setContentSize({ 300.0f, 27.0f });

            auto select = FileSelectNode::create(300.0f);
            select->setAnchorPoint({ 0.f, 0.f });
            select->setPosition({ 0.f, 0.f });

            if (!initialFilename.empty()) {
                select->preloadFilename(initialFilename);
            }

            select->setOnFileSelected([selected, previewRef](std::filesystem::path const& path) {
                auto result = FileValueMenu::create(path);
                if (result.isErr()) {
                    FLAlertLayer::create(
                        "Error",
                        fmt::format("Failed to read file: {}", result.unwrapErr()),
                        "Ok"
                    )->show();
                    return;
                }

                std::string code = result.unwrap().getContents();

                gd::string encoded = LevelTools::base64EncodeString(
                    gd::string(code.c_str(), code.size())
                );
                std::string filename = path.filename().string();

                for (auto* obj : selected) {
                    if (auto* trig = geode::cast::typeinfo_cast<LuaTrigger*>(obj)) {
                        trig->m_b64code  = std::string(encoded.c_str(), encoded.size());
                        trig->m_filename = filename;
                        trig->checkMod();
                    }
                }

                if (*previewRef) {
                    (*previewRef)->setText(code);
                }
            });

            menu->addChild(select);
            return menu;
        })
        .build();

    return PopupConfig::builder()
        .width(420)
        .height(260)
        .gapY(20)
        .title("Lua Code Editor")
        .info(InfoPopup::builder()
            .title("Help")
            .description("This trigger lets you write <cl>Lua</c> code in GD, giving you loads of flexibility "
                "in what you want your level to do.\n\nUpload your Lua files using the button at the bottom of "
                "the trigger menu.\n\n<cy>For full trigger docs, click the \"Open Docs\" button to the right!</c>")
            .build())
        .menu(std::move(luaEditor))
        .menu(editor_popup::CustomValueMenu::builder()
            .id("utils")
            .title("Utilities")
            .factory([](const Selected&, Popup*) -> CCMenu* {
                auto menu = CCMenu::create();
                menu->setContentSize({ 300.f, 90.f });

                auto docsBtn = CCMenuItemExt::createSpriteExtra(
                    ButtonSprite::create("Open Docs", 180, true, "goldFont.fnt", "GJ_button_01.png", 26.f, 0.6f),
                    [](CCObject*) {
                        utils::web::openLinkInBrowser("https://gdresources.omgrod.me/lua/index");
                    }
                );

                auto bugBtn = CCMenuItemExt::createSpriteExtra(
                    ButtonSprite::create("Report Bug", 180, true, "goldFont.fnt", "GJ_button_01.png", 26.f, 0.6f),
                    [](CCObject*) {
                        utils::web::openLinkInBrowser("https://github.com/OmgRod/LuaTrigger/issues/new?template=bug_report.md");
                    }
                );

                auto featureBtn = CCMenuItemExt::createSpriteExtra(
                    ButtonSprite::create("Request Feature", 180, true, "goldFont.fnt", "GJ_button_01.png", 26.f, 0.6f),
                    [](CCObject*) {
                        utils::web::openLinkInBrowser("https://github.com/OmgRod/LuaTrigger/issues/new?template=feature_request.md");
                    }
                );

                menu->addChild(docsBtn);
                menu->addChild(bugBtn);
                menu->addChild(featureBtn);

                menu->setLayout(
                    AxisLayout::create(Axis::Column)
                        ->setGap(5.0f)
                        ->setAxisReverse(true)
                );

                menu->updateLayout();
                return menu;
            })
            .build())
        .menu(std::move(fileUpload))
        .triggerToggles(true)
        .build();
}

void LuaTrigger::postInit() {
    this->setHitbox({ 1, 1 });
    this->checkMod();
}

void LuaTrigger::triggerObject(
    GJBaseGameLayer* layer,
    const int uniqueID,
    const gd::vector<int>* remapKeys
) {
    if (!m_active)
        return;

    auto code = LevelTools::base64DecodeString(m_b64code);
    m_luaManager.executeScript(code);

    CustomObject::triggerObject(layer, uniqueID, remapKeys);
}

bool LuaTrigger::ignoreEditorDuration() {
    return true;
}

void LuaTrigger::checkMod() {
    m_active = !m_b64code.empty();
}

$on_mod(Loaded) {
    ObjectAPI::registerObject(ObjectInfo::builder()
        .id("lua-trigger"_spr)
        .sprite("trigger.png"_spr)
        .construction(ComplexObject::builder()
            .factory(LuaTrigger::create)
            .customProperties({
                PropertyInterface::from(LuaTrigger::SCRIPT, &LuaTrigger::m_b64code, std::string("")),
                PropertyInterface::from(LuaTrigger::FILENAME, &LuaTrigger::m_filename, std::string("")),
            })
            .build())
        .editObject(LuaTrigger::getEditObjectConfig)
        .editorTab(EditorTab::Triggers)
        .build());
}
