#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Code partially taken from Geode

class FileValueMenu {
public:
    FileValueMenu() = default;
    explicit FileValueMenu(std::filesystem::path path);

    static Result<FileValueMenu> create(std::filesystem::path path);

    [[nodiscard]] std::string const& getContents() const;

    Result<> setPath(std::filesystem::path path);
    Result<> reload();

private:
    Result<> loadFromDisk();

    std::filesystem::path m_path;
    std::string m_contents;
    bool m_loaded = false;
};
