#pragma once

#include <Geode/Geode.hpp>

namespace file_value_menu {

using namespace geode::prelude;

class FileValueMenu {
public:
    FileValueMenu() = default;
    explicit FileValueMenu(std::filesystem::path path);

    static Result<FileValueMenu> create(std::filesystem::path path);

    [[nodiscard]] std::filesystem::path const& getPath() const;
    [[nodiscard]] std::string const& getContents() const;
    [[nodiscard]] bool hasContents() const;

    Result<> setPath(std::filesystem::path path);
    Result<> reload();
    Result<> save(std::string const& contents);

    [[nodiscard]] std::string getFilename() const;
    [[nodiscard]] std::string getExtension() const;
    [[nodiscard]] bool exists() const;
    [[nodiscard]] bool isDirectory() const;

private:
    Result<> loadFromDisk();

    std::filesystem::path m_path;
    std::string m_contents;
    bool m_loaded = false;
};

}
