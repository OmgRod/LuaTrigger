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

    std::error_code ec;
    if (!std::filesystem::exists(m_path, ec)) {
        m_contents.clear();
        m_loaded = false;
        return Err("File does not exist: {}", m_path.string());
    }

    if (std::filesystem::is_directory(m_path, ec)) {
        m_contents.clear();
        m_loaded = false;
        return Err("Path is a directory, not a file: {}", m_path.string());
    }

    std::ifstream file(m_path, std::ios::binary);
    if (!file.is_open()) {
        m_contents.clear();
        m_loaded = false;
        return Err("Failed to open {} for reading", m_path.string());
    }

    m_contents.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (file.bad()) {
        m_contents.clear();
        m_loaded = false;
        return Err("Failed to read the full contents of {}", m_path.string());
    }

    m_loaded = true;
    return Ok();
}
