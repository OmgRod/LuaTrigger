#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class FileSelectNode : public CCNode {
protected:
    CCLabelBMFont* m_nameLabel;
    async::TaskHolder<Result<std::optional<std::filesystem::path>>> m_pickListener;
    CCMenuItemSpriteExtra* m_selectBtn;
    CCSprite* m_selectBtnSpr;
    std::filesystem::path m_path;
    std::function<void(std::filesystem::path const&)> m_onFileSelected;

    bool init(float width);
    void updateState();
    void onPickFile(CCObject*);
    void setPath(std::filesystem::path path);

public:
    static FileSelectNode* create(float width);
    std::filesystem::path const& getPath() const { return m_path; }
    void setOnFileSelected(std::function<void(std::filesystem::path const&)> callback);
    void preloadFilename(std::string const& filename);
};
