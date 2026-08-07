#include "Geode/utils/random.hpp"
#include <ExecuteLuaTrigger.hpp>
#include <utils/Utils.hpp>
#include <nodes/ExamplesPopup.hpp>

ExecuteLuaTrigger::ExecuteLuaTrigger(ObjectInfo* info) : CustomObject(info, ObjectTraits::builder()
    .gameObjectType(GameObjectType::Modifier)
    .ignoreEditorDuration(true)
    .build()) {}

ExecuteLuaTrigger* ExecuteLuaTrigger::create(ObjectInfo* info) {
    return new ExecuteLuaTrigger(info);
}

std::string ExecuteLuaTrigger::highlightSyntax(const std::string& code) {
    std::string result;
    size_t i = 0;

    while (i < code.size()) {
        if (i + 1 < code.size() && code[i] == '-' && code[i + 1] == '-') {
            result += "<c-808080>";
            
            while (i < code.size() && code[i] != '\n' && code[i] != '\r') {
                result += code[i];
                i++;
            }
            
            result += "</c>";
            continue;
        }

        if (code[i] == '"' || code[i] == '\'') {
            char quote = code[i];
            result += fmt::format("<c-FFFACD>{}", quote);
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
                result += fmt::format("<c-ADD8E6>{}</c>", word);
            } else if (isFunction(word, code, i)) {
                result += fmt::format("<c-90EE90>{}</c>", word);
            } else if (isClass(word)) {
                result += fmt::format("<c-FFB6C1>{}</c>", word);
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
            result += fmt::format("<c-FFDAB9>{}</c>", num);
            continue;
        }

        result += code[i];
        i++;
    }

    return result;
}

PopupOptions ExecuteLuaTrigger::getEditObjectConfig(const Selected& selected) {
    std::string initialCode = "print(\"Upload a .lua file to see the preview here...\")";
    std::string initialFilename = "";

    ExecuteLuaTrigger* triggerInstance;
    if (!selected.empty()) {
        triggerInstance = typeinfo_cast<ExecuteLuaTrigger*>(selected[0]);
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
                if (path.empty()) {
                    FLAlertLayer::create("Error", "No file path has been set", "OK")->show();
                    return;
                }

                auto result = utils::file::readString(path);
                if (result.isErr()) {
                    FLAlertLayer::create(
                        "Error",
                        fmt::format("Failed to read {}: {}", 
                            utils::string::pathToString(path),
                            result.unwrapErr()
                        ),
                        "OK"
                    )->show();
                    return;
                }

                std::string code = result.unwrap();

                gd::string encoded = LevelTools::base64EncodeString(gd::string(code.c_str(), code.size()));
                std::string filename = path.filename().string();

                for (auto* obj : selected) {
                    if (auto* trig = cast::typeinfo_cast<ExecuteLuaTrigger*>(obj)) {
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
        .title("Execute Lua Code")
        .info(InfoPopup::builder()
            .title("Help")
            .description("Execute <cg>sandboxed Lua code</c> right inside GD to bring complex mechanics and endless flexibility to your levels!\n\n"
                "Upload your files using the button in the bottom-left of the trigger popup.\n\n"
                "Click <cy>\"Open Docs\"</c> for full documentation or <cd>\"View Examples\"</c> for sample scripts!")
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

                auto tipOfTheDayBtn = CCMenuItemExt::createSpriteExtra(
                    ButtonSprite::create("Useful Tips"),
                    [](CCObject*) {
                        static std::vector<std::string> tips = {
                            "Use <cy>state</c> to use persistent variables, e.g. <cl>`state.value = 123`</c>.\nThe values of <cp>these variables will persist across attempts</c> (but will <cr>reset</c> once the level is <cf>exited</c>).",
                            "Use <cy>clearState()</c> to clear all persistent variables.",
                            "Use <cr>Player.kill()</c> to kill the player.",
                            "Use <cg>Popup.show()</c> to show a GD popup.",
                            "Press <cl>Shift+T</c> to open the <cg>Debug Console</c>.",
                            "Type <co>print(\"Hello, world!\", true)</c> to create a \"silent log\" which only appears in the debug console and doesn't show a notification.",
                        };

                        static size_t currentTipIndex = 0;

                        std::string tip = tips[currentTipIndex];
                        
                        FLAlertLayer::create(
                            fmt::format("Tip {}/{}", currentTipIndex + 1, tips.size()).c_str(),
                            tip,
                            "OK"
                        )->show();

                        currentTipIndex = (currentTipIndex + 1) % tips.size();
                    }
                );

                auto menu = CCMenu::create();
                menu->setContentSize({ 300.f, 150.f });

                menu->addChild(docsBtn);
                menu->addChild(bugBtn);
                menu->addChild(featureBtn);
                menu->addChild(examplesBtn);
                menu->addChild(tipOfTheDayBtn);
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
        .menu(ToggleMenu::builder()
            .id("ignore-timeout")
            .title("Ignore Timeout")
            .onValue([selected](bool value, const Selected& sel, Popup*) {
                applyValueToSelected(sel, &ExecuteLuaTrigger::m_ignoreTimeout, value);
            })
            .currentValue([selected](const Selected& sel, Popup*) -> bool {
                return getCommonValueOrDefault(sel, &ExecuteLuaTrigger::m_ignoreTimeout);
            })
            .build())
        .triggerToggles(true)
        .build();
}

void ExecuteLuaTrigger::postInit() {
    this->setHitbox({ 1, 1 });
    this->checkMod();
    this->setupResetListener();
}

ExecuteLuaTrigger::~ExecuteLuaTrigger() {
    if (auto* pl = PlayLayer::get()) {
        LuaInterpreter::cleanupLayer(pl);
    } else if (auto* g = GJBaseGameLayer::get()) {
        LuaInterpreter::cleanupLayer(g);
    }
}

void ExecuteLuaTrigger::triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) {
    auto interp = LuaInterpreter::forLayer(layer);
    if (!interp) return;

    if (!m_b64code.empty()) {
        std::string rawLuaCode = LevelTools::base64DecodeString(m_b64code);
        interp->runString(rawLuaCode, m_ignoreTimeout);
    }

    CustomObject::triggerObject(layer, uniqueID, remapKeys);
}

void ExecuteLuaTrigger::checkMod() {
    m_active = !m_b64code.empty();
}

$on_mod(Loaded) {
    ObjectAPI::registerObject(ObjectInfo::builder()
        .id("lua-trigger"_spr)
        .sprite("execute.png"_spr)
        .construction(ComplexObject::builder()
            .factory(ExecuteLuaTrigger::create)
            .customProperties({
                PropertyInterface::from(ExecuteLuaTrigger::SCRIPT,         &ExecuteLuaTrigger::m_b64code,       std::string("")),
                PropertyInterface::from(ExecuteLuaTrigger::FILENAME,       &ExecuteLuaTrigger::m_filename,     std::string("")),
                PropertyInterface::from(ExecuteLuaTrigger::IGNORE_TIMEOUT, &ExecuteLuaTrigger::m_ignoreTimeout, false),
            })
            .build())
        .editObject(ExecuteLuaTrigger::getEditObjectConfig)
        .editorTab(EditorTab::Triggers)
        .build());
}

void ExecuteLuaTrigger::stopLua() {
    this->stopAllActions();
    if (auto* pl = PlayLayer::get()) {
        if (auto interp = LuaInterpreter::forLayer(pl)) interp->stop();
    }
}

void ExecuteLuaTrigger::pauseLua() {
    this->pauseSchedulerAndActions();
    if (auto* pl = PlayLayer::get()) {
        if (auto interp = LuaInterpreter::forLayer(pl)) interp->pause();
    }
}

void ExecuteLuaTrigger::resumeLua() {
    this->resumeSchedulerAndActions();
    if (auto* pl = PlayLayer::get()) {
        if (auto interp = LuaInterpreter::forLayer(pl)) interp->resume();
    }
}

void ExecuteLuaTrigger::resetLuaState() {
    log::info("Level reset event received! Resetting trigger state...");
    this->stopAllActions();
    if (auto* pl = PlayLayer::get()) {
        if (auto interp = LuaInterpreter::forLayer(pl)) interp->resetState();
    }
}

void ExecuteLuaTrigger::setupResetListener() {
    m_resetListener = LevelResetEvent().listen([this]() {
        this->resetLuaState();
        return true;
    });
}