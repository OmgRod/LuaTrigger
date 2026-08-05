#include <Geode/Geode.hpp>

using namespace geode::prelude;

struct ExampleScript : public CCObject, public TableViewCellDelegate {
    std::string name;
    std::string description;
    std::string filename;
    bool locked;
};

class ExamplesPopup : public geode::Popup {
protected:
    LuaTrigger* m_trigger;

    bool init(LuaTrigger* trigger, std::function<void(const std::string&)> onCodeSelected);

public:
    static ExamplesPopup* create(LuaTrigger* trigger, std::function<void(const std::string&)> onCodeSelected);

    void closePopup() {
        this->onClose(nullptr);
    }
};

class ExampleCell : public CCLayer {
    bool init(LuaTrigger* trigger, ExamplesPopup* popup, ExampleScript* script, std::function<void(const std::string&)> onCodeSelected);
public:
    static ExampleCell* create(LuaTrigger* trigger, ExamplesPopup* popup, ExampleScript* script, std::function<void(const std::string&)> onCodeSelected);
};
