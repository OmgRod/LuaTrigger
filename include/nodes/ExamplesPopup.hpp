#include <Geode/Geode.hpp>
#include <ExecuteLuaTrigger.hpp>

using namespace geode::prelude;

struct ExampleScript : public CCObject, public TableViewCellDelegate {
    std::string name;
    std::string description;
    std::string filename;
    bool locked;
};

class ExamplesPopup : public geode::Popup {
protected:
    ExecuteLuaTrigger* m_trigger;

    bool init(ExecuteLuaTrigger* trigger, std::function<void(const std::string&)> onCodeSelected);

public:
    static ExamplesPopup* create(ExecuteLuaTrigger* trigger, std::function<void(const std::string&)> onCodeSelected);

    void closePopup() {
        this->onClose(nullptr);
    }
};

class ExampleCell : public CCLayer {
    bool init(ExecuteLuaTrigger* trigger, ExamplesPopup* popup, ExampleScript* script, std::function<void(const std::string&)> onCodeSelected);
public:
    static ExampleCell* create(ExecuteLuaTrigger* trigger, ExamplesPopup* popup, ExampleScript* script, std::function<void(const std::string&)> onCodeSelected);
};
