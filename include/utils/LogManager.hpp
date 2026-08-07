#pragma once

#include <Geode/Geode.hpp>

enum class LogLevel {
    Info,
    Warn,
    Error
};

struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string message;
    std::string rawFormatted;
    std::string colorFormatted;
};

class LogManager {
public:
    using LogCallback = std::function<void(const LogEntry&)>;

private:
    std::vector<LogEntry> m_logs;
    std::vector<LogCallback> m_callbacks;

    LogManager() = default;
    std::string getCurrentTimeString() const;

public:
    static LogManager& get();

    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    void log(LogLevel level, const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    const std::vector<LogEntry>& getLogs() const;
    void addCallback(LogCallback cb);
    void clearLogs();
};
