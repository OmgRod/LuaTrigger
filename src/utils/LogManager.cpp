#include "utils/LogManager.hpp"

// i would've used fmt time stuff here but my compiler hates me
std::string LogManager::getCurrentTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
#ifdef GEODE_IS_WINDOWS
    localtime_s(&timeInfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeInfo);
#endif
    std::stringstream ss;
    ss << std::put_time(&timeInfo, "%H:%M:%S");
    return ss.str();
}

LogManager& LogManager::get() {
    static LogManager instance;
    return instance;
}

void LogManager::log(LogLevel level, const std::string& message) {
    std::string timeStr = getCurrentTimeString();
    std::string levelStr;
    std::string colorTag;

    switch (level) {
        case LogLevel::Info:
            levelStr = "INFO";
            colorTag = "c-bbbbbb";
            geode::log::info("{}", message);
            break;
        case LogLevel::Warn:
            levelStr = "WARN";
            colorTag = "c-cccc00";
            geode::log::warn("{}", message);
            break;
        case LogLevel::Error:
            levelStr = "ERROR";
            colorTag = "c-ff2222";
            geode::log::error("{}", message);
            break;
    }

    std::string rawFormatted = fmt::format("[{}] {}: {}", timeStr, levelStr, message);
    std::string colorFormatted = fmt::format("<{}>{}</c>", colorTag, rawFormatted);

    LogEntry entry{level, timeStr, message, rawFormatted, colorFormatted};
    m_logs.push_back(entry);

    for (const auto& cb : m_callbacks) {
        if (cb) {
            cb(entry);
        }
    }
}

void LogManager::info(const std::string& message) {
    log(LogLevel::Info, message);
}

void LogManager::warn(const std::string& message) {
    log(LogLevel::Warn, message);
}

void LogManager::error(const std::string& message) {
    log(LogLevel::Error, message);
}

const std::vector<LogEntry>& LogManager::getLogs() const {
    return m_logs;
}

void LogManager::addCallback(LogCallback cb) {
    m_callbacks.push_back(cb);
}

void LogManager::clearLogs() {
    m_logs.clear();
}
