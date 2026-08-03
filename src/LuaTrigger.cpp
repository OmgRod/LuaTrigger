#include <LuaTrigger.hpp>

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

inline void LuaTrigger::setupLuaInterpreter() {
    m_lua = std::make_unique<sol::state>();
    m_lua->open_libraries( // THIS MUST BE APPROPRIATE PACKAGES ONLY! WE DONT WANT OUR USERS GETTING HACKED!
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::bit32,
        sol::lib::coroutine,
        sol::lib::table,
        sol::lib::utf8
    );

    m_persistentState = m_lua->create_table();
    m_lua->globals()["state"] = m_persistentState;

    GJBaseGameLayer* layer = nullptr;
    if (auto playLayer = PlayLayer::get()) layer = playLayer;
    // else if (auto editorLayer = LevelEditorLayer::get()) layer = editorLayer; // no editor layer because its weird af
    if (!layer) return;
    
    sol::table playerTable = m_lua->create_named_table("Player", 
        "Player1", 1,
        "Player2", 2,
        "Both", 3
    );

    playerTable["kill"] = [layer](sol::optional<int> playerType) {
        int type = playerType.value_or(1);

        auto killPlayer = [&](PlayerObject* p) {
            if (p) layer->destroyPlayer(p, nullptr);
        };

        if (type == 1 || type == 3) killPlayer(layer->m_player1);
        if (type == 2 || type == 3) killPlayer(layer->m_player2);
    };

    sol::table popupTable = m_lua->create_named_table("Popup");
    
    popupTable["show"] = [layer](std::string title, std::string content, sol::optional<std::string> btn1, sol::optional<std::string> btn2, sol::optional<sol::function> callback, sol::optional<bool> doShow, sol::optional<bool> cancelledByEscape) {
        std::shared_ptr<sol::protected_function> luaCallback;

        if (callback.has_value() && callback->valid()) {
            luaCallback = std::make_shared<sol::protected_function>(*callback);
        }

        togglePlayerMovement(layer, true);

        std::function<void(FLAlertLayer*, bool)> callbackWrapper =
            [luaCallback, layer](FLAlertLayer* popup, bool secondButton) {
                togglePlayerMovement(layer, false);

                if (!luaCallback) return;

                auto result = (*luaCallback)(popup, secondButton);

                if (!result.valid()) {
                    sol::error err = result;
                    log::error("Lua popup callback error: {}", err.what());
                }
            };

        std::string button1 = btn1.value_or("OK");
        std::string button2 = btn2.value_or("");

        if (button2.empty()) {
            return createQuickPopup(
                title.c_str(),
                content,
                button1.c_str(),
                nullptr,
                callbackWrapper,
                doShow.value_or(true),
                cancelledByEscape.value_or(false)
            );
        }

        return createQuickPopup(
            title.c_str(),
            content,
            button1.c_str(),
            button2.c_str(),
            callbackWrapper,
            doShow.value_or(true),
            cancelledByEscape.value_or(false)
        );
    };

    sol::table groupTable = m_lua->create_named_table("Object");
    
    groupTable["move"] = [layer](int groupID, float dx, float dy, sol::optional<float> duration) {
        log::info("Lua Object.move({}, {}, {}, {})",
            groupID, dx, dy, duration.value_or(0.f)
        );

        float dur = duration.value_or(0.0f);

        moveGroupWithEasing(layer, groupID, { dx, dy }, dur);
    };

    groupTable["rotate"] = [layer](int targetGroupID, int centerGroupID, int degrees, sol::optional<int> times360, sol::optional<float> duration, sol::optional<int> easingType, sol::optional<float> easingRate) {
        log::info("Lua Object.rotate({}, {}, {}, {})", targetGroupID, centerGroupID, degrees, duration.value_or(0.f));

        rotateGroupWithEasing(layer, targetGroupID, centerGroupID, degrees, times360.value_or(0), duration.value_or(0.0f), easingType.value_or(0), easingRate.value_or(2.0f));
    };

    groupTable["scale"] = [layer](int targetGroupID, int centerGroupID, float scaleX, sol::optional<float> scaleY, sol::optional<float> duration, sol::optional<int> easingType, sol::optional<float> easingRate, sol::optional<sol::table> options) {
        log::info("Lua Group.scale({}, {}, {}, {})", targetGroupID, centerGroupID, scaleX, duration.value_or(0.f));

        float sy = scaleY.value_or(scaleX);
        float dur = duration.value_or(0.0f);

        bool divByX = false;
        bool divByY = false;
        bool onlyMove = false;
        bool relativeScale = false;
        bool relativeRotation = false;

        if (options.has_value()) {
            auto opts = options.value();
            divByX = opts.get_or("divByX", false);
            divByY = opts.get_or("divByY", false);
            onlyMove = opts.get_or("onlyMove", false);
            relativeScale = opts.get_or("relativeScale", false);
            relativeRotation = opts.get_or("relativeRotation", false);
        }

        scaleGroupWithEasing(layer, targetGroupID, centerGroupID, scaleX, sy, dur, easingType.value_or(0), easingRate.value_or(2.0f), divByX, divByY, onlyMove, relativeScale, relativeRotation);
    };
    // Not ready yet!
    /*m_lua->new_usertype<LuaObject>(
        "LuaObject",

        "addToLayer",
        &LuaObject::addToLayer,

        "setPosition",
        &LuaObject::setPosition,
        "getPosition",
        &LuaObject::getPosition,
        "move",
        &LuaObject::move,


        "setRotation",
        &LuaObject::setRotation,
        "getRotation",
        &LuaObject::getRotation,
        "rotate",
        &LuaObject::rotate,


        "setScale",
        &LuaObject::setScale,
        "getScale",
        &LuaObject::getScale,
        "scale",
        &LuaObject::scale,


        "getID",
        &LuaObject::getID
    );
    groupTable["new"] = [layer](int id) -> LuaObject* {
        auto obj = new LuaObject(layer,id);
        obj->addToLayer();
        return obj;
    };*/

    m_lua->set_function("wait", [](double seconds, sol::this_state s) {
        lua_pushnumber(s, seconds);
        return lua_yield(s, 1);
    });

    m_lua->set_function("print", [](sol::variadic_args args) {
        auto text = formatLuaArgs(args);

        log::info("[Lua] {}", text);
        notifapi::info(text);
    });
    m_lua->set_function("error", [](sol::variadic_args args) {
        auto text = formatLuaArgs(args);

        log::error("[Lua] {}", text);
        notifapi::error(text);
    });
    m_lua->set_function("warn", [](sol::variadic_args args) {
        auto text = formatLuaArgs(args);

        log::warn("[Lua] {}", text);
        notifapi::warn(text);
    });

    m_lua->set_function("clearState", [this]() {
        m_persistentState = m_lua->create_table();
        m_lua->globals()["state"] = m_persistentState;
    });
}

LuaTrigger::LuaTrigger(ObjectInfo* info) : CustomObject(info, GameObjectType::Modifier) {
    setupLuaInterpreter();
}

LuaTrigger* LuaTrigger::create(ObjectInfo* info) {
    return new LuaTrigger(info);
}

std::string LuaTrigger::highlightSyntax(const std::string& code) {
    std::string result;
    size_t i = 0;
    while(i < code.size()) {
        if (code[i] == '"' || code[i] == '\'') {
            char quote = code[i];
            result += fmt::format("<c#FFFACD>{}", quote);
            i++;
            while(i < code.size() && code[i] != quote) {
                result += code[i];
                i++;
            }
            if(i < code.size()) { result += code[i]; i++; }
            result += "</c>";
            continue;
        }

        if (std::isalpha(code[i]) || code[i] == '_') {
            std::string word;
            size_t start = i;
            while(i < code.size() && (std::isalnum(code[i]) || code[i] == '_')) {
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

        if (std::isdigit(code[i])) {
            std::string num;
            while(i < code.size() && (std::isdigit(code[i]) || code[i] == '.')) {
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
    this->setupResetListener();
}

void LuaTrigger::triggerObject(GJBaseGameLayer* layer, const int uniqueID, const gd::vector<int>* remapKeys) {
    if (!m_active) return;

    m_executionToken++;
    m_disabled = false;

    if (!m_lua) {
        log::info("Re-initializing Lua interpreter state...");
        setupLuaInterpreter();
    }

    auto code = LevelTools::base64DecodeString(m_b64code);
    if (code.empty()) return;

    sol::environment env = createScriptEnvironment();

    auto loaded = m_lua->load("local _ENV = ...\n" + code);
    if (!loaded.valid()) {
        sol::error err = loaded;
        log::error("Lua Load Error: {}", err.what());
        return;
    }

    auto coroutine = std::make_shared<sol::coroutine>(loaded);
    runCoroutine(coroutine, env, m_executionToken);

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
    log::info("[LuaTrigger] Level reset event received! Resetting trigger state...");

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