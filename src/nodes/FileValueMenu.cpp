#include "FileValueMenu.hpp"

FileValueMenu::FileValueMenu(std::filesystem::path path)
    : m_path(std::move(path)) {
}

Result<FileValueMenu> FileValueMenu::create(std::filesystem::path path) {
    auto menu = FileValueMenu(std::move(path));
    if (auto const result = menu.reload(); result.isErr()) {
        return Err(result.unwrapErr());
    }

    return Ok(std::move(menu));
}

std::filesystem::path const& FileValueMenu::getPath() const {
    return m_path;
}

std::string const& FileValueMenu::getContents() const {
    return m_contents;
}

bool FileValueMenu::hasContents() const {
    return m_loaded;
}

Result<> FileValueMenu::setPath(std::filesystem::path path) {
    m_path = std::move(path);
    return reload();
}

Result<> FileValueMenu::reload() {
    return loadFromDisk();
}

Result<> FileValueMenu::save(std::string const& contents) {
    if (m_path.empty()) {
        return Err("No file path has been set");
    }

    std::error_code ec;
    auto parent = m_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return Err("Failed to create parent directories for {}: {}", m_path.string(), ec.message());
        }
    }

    std::ofstream file(m_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return Err("Failed to open {} for writing", m_path.string());
    }

    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file.good()) {
        return Err("Failed to write the full file contents to {}", m_path.string());
    }

    m_contents = contents;
    m_loaded = true;
    return Ok();
}

std::string FileValueMenu::getFilename() const {
    return m_path.filename().string();
}

std::string FileValueMenu::getExtension() const {
    return m_path.extension().string();
}

bool FileValueMenu::exists() const {
    std::error_code ec;
    return std::filesystem::exists(m_path, ec);
}

bool FileValueMenu::isDirectory() const {
    std::error_code ec;
    return std::filesystem::is_directory(m_path, ec);
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
