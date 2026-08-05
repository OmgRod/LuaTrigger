#include <nodes/FileValueMenu.hpp>

FileValueMenu::FileValueMenu(std::filesystem::path path) : m_path(std::move(path)) { }

Result<FileValueMenu> FileValueMenu::create(std::filesystem::path path) {
    auto menu = FileValueMenu(std::move(path));
    if (auto const result = menu.reload(); result.isErr()) {
        return Err(result.unwrapErr());
    }

    return Ok(std::move(menu));
}

std::string const& FileValueMenu::getContents() const {
    return m_contents;
}

Result<> FileValueMenu::reload() {
    return loadFromDisk();
}

Result<> FileValueMenu::loadFromDisk() {
    if (m_path.empty()) {
        m_contents.clear();
        m_loaded = false;
        return Err("No file path has been set");
    }

    auto result = geode::utils::file::readString(m_path);

    if (result.isErr()) {
        m_contents.clear();
        m_loaded = false;
        return Err("Failed to read {}: {}", 
            geode::utils::string::pathToString(m_path),
            result.unwrapErr()
        );
    }

    m_contents = result.unwrap();
    m_loaded = true;

    return Ok();
}
