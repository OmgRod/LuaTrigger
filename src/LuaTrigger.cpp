#include <LuaTrigger.hpp>
#include <utils/Utils.hpp>
#include <nodes/ExamplesPopup.hpp>

sol::environment LuaTrigger::createScriptEnvironment() {
    sol::environment env(*m_lua, sol::create);

    env["state"] = m_persistentState;

    auto meta = m_lua->create_table();
    meta["__index"] = m_lua->globals();

    env[sol::metatable_key] = meta;

    return env;
}

inline void LuaTrigger::runCoroutine(std::shared_ptr<sol::coroutine> coroutine, sol::environment env, uint32_t token) {
    if (m_disabled || !m_lua || !coroutine->runnable() || token != m_executionToken) {
        return;
    }

    auto result = (*coroutine)(env);

    if (!result.valid()) {
        sol::error err = result;
        log::error("Lua Coroutine Error: {}", err.what());
        return;
    }

    if (m_disabled || !m_lua || !coroutine->runnable() || token != m_executionToken) {
        return;
    }

    if (result.return_count() > 0) {
        float seconds = result.get<float>(0);

        cocos2d::CCNode* targetNode = PlayLayer::get();
        if (!targetNode) {
            targetNode = cocos2d::CCDirector::sharedDirector()->getRunningScene();
        }

        if (!targetNode) return;

        targetNode->runAction(
            cocos2d::CCSequence::create(
                cocos2d::CCDelayTime::create(seconds),
                CallFuncExt::create(
                    [this, coroutine, env, token]() mutable {
                        if (this->m_disabled || !this->m_lua || token != this->m_executionToken) {
                            return;
                        }
                        this->runCoroutine(coroutine, env, token); 
                    }
                ),
                nullptr
            )
        );
    }
}

LuaTrigger::LuaTrigger(ObjectInfo* info) : CustomObject(info, GameObjectType::Modifier) {}

LuaTrigger* LuaTrigger::create(ObjectInfo* info) {
    return new LuaTrigger(info);
}

std::string LuaTrigger::highlightSyntax(const std::string& code) {
    std::string result;
    size_t i = 0;

    while (i < code.size()) {
        if (i + 1 < code.size() && code[i] == '-' && code[i + 1] == '-') {
            result += "<c#808080>";
            
            while (i < code.size() && code[i] != '\n' && code[i] != '\r') {
                result += code[i];
                i++;
            }
            
            result += "</c>";
            continue;
        }

        if (code[i] == '"' || code[i] == '\'') {
            char quote = code[i];
            result += fmt::format("<c#FFFACD>{}", quote);
            i++;

            while (i < code.size()) {
                if (code[i] == '\\' && i + 1 < code.size()) {
                    result += code[i];
                    result += code[i + 1];
                    i += 2;
                    continue;
                }

                if (code[i] == quote) {
                    result += code[i];
                    i++;
                    break;
                }

                if (code[i] == '\n' || code[i] == '\r') {
                    break;
                }

                result += code[i];
                i++;
            }

            result += "</c>";
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(code[i])) || code[i] == '_') {
            std::string word;
            while (i < code.size() && (std::isalnum(static_cast<unsigned char>(code[i])) || code[i] == '_')) {
                word += code[i];
                i++;
            }

            if (isKeyword(word)) {
                result += fmt::format("<c#ADD8E6>{}</c>", word);
            } else if (isFunction(word, code, i)) {
                result += fmt::format("<c#90EE90>{}</c>", word);
            } else if (isClass(word)) {
                result += fmt::format("<c#FFB6C1>{}</c>", word);
            } else {
                result += word;
            }
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(code[i]))) {
            std::string num;
            while (i < code.size() && (std::isdigit(static_cast<unsigned char>(code[i])) || code[i] == '.')) {
                num += code[i];
                i++;
            }
            result += fmt::format("<c#FFDAB9>{}</c>", num);
            continue;
        }

        result += code[i];
        i++;
    }

    return result;
}

PopupOptions LuaTrigger::getEditObjectConfig(const Selected& selected) {
    std::string initialCode = "print(\"Upload a .lua file to see the preview here...\")";
    std::string initialFilename = "";

    LuaTrigger* triggerInstance = nullptr;
    if (!selected.empty()) {
        triggerInstance = geode::cast::typeinfo_cast<LuaTrigger*>(selected[0]);
    }

    if (triggerInstance) {
        if (!triggerInstance->m_b64code.empty()) {
            initialCode = LevelTools::base64DecodeString(triggerInstance->m_b64code);
        }
        initialFilename = triggerInstance->m_filename;
    }

    auto previewRef = std::make_shared<amber::ScrollTextArea*>(nullptr);

    auto luaEditor = editor_popup::CustomValueMenu::builder()
        .id("lua-preview")
        .title("Code Preview")
        .factory([initialCode, previewRef](const Selected& /*selected*/, Popup* /*popup*/) -> CCMenu* {
            auto menu = CCMenu::create();
            menu->setContentSize({ 300.0f, 150.0f });

            std::string highlightedCode = highlightSyntax(initialCode);

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

                gd::string encoded = LevelTools::base64EncodeString(gd::string(code.c_str(), code.size()));
                std::string filename = path.filename().string();

                for (auto* obj : selected) {
                    if (auto* trig = geode::cast::typeinfo_cast<LuaTrigger*>(obj)) {
                        trig->m_b64code  = std::string(encoded.c_str(), encoded.size());
                        trig->m_filename = filename;
                        trig->checkMod();

                        if (*previewRef) {
                            (*previewRef)->setText(trig->highlightSyntax(code));
                        }
                    }
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
            .factory([triggerInstance, previewRef](const Selected&, Popup*) -> CCMenu* {
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

                auto examplesBtn = CCMenuItemExt::createSpriteExtra(
                    ButtonSprite::create("View Examples", 180, true, "goldFont.fnt", "GJ_button_01.png", 26.f, 0.6f),
                    [triggerInstance, previewRef](CCObject*) {
                        if (triggerInstance) {
                            ExamplesPopup::create(triggerInstance, [triggerInstance, previewRef](const std::string& newCode) {
                                if (*previewRef) {
                                    (*previewRef)->setText(triggerInstance->highlightSyntax(newCode));
                                }
                            })->show();
                        }
                    }
                );

                auto menu = CCMenu::create();
                menu->setContentSize({ 300.f, 150.f });

                menu->addChild(docsBtn);
                menu->addChild(bugBtn);
                menu->addChild(featureBtn);
                menu->addChild(examplesBtn);
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
    this->setupResetListener();
}

void LuaTrigger::triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) {
    interpreter.init(layer);

    if (!this->m_b64code.empty()) {
        std::string rawLuaCode = LevelTools::base64DecodeString(this->m_b64code);
        interpreter.runString(rawLuaCode);
    }

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
        .sprite("execute.png"_spr)
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

void LuaTrigger::stopLua() {
    m_disabled = true;
    m_executionToken++;

    this->stopAllActions();

    if (m_lua) {
        m_persistentState = m_lua->create_table();
        m_lua->globals()["state"] = m_persistentState;
    }
}

void LuaTrigger::pauseLua() {
    m_disabled = true;
    this->pauseSchedulerAndActions();
}

void LuaTrigger::resumeLua() {
    m_disabled = false;
    this->resumeSchedulerAndActions();
}

void LuaTrigger::resetLuaState() {
    log::info("Level reset event received! Resetting trigger state...");

    m_disabled = false;
    m_executionToken++;

    this->stopAllActions();

    if (m_lua) {
        m_persistentState = m_lua->create_table();
        m_lua->globals()["state"] = m_persistentState;
    }
}

void LuaTrigger::setupResetListener() {
    m_resetListener = LevelResetEvent().listen([this]() {
        this->resetLuaState();
        return true;
    });
}