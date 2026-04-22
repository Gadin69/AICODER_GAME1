#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(LogLevel level) {
        minLevel = level;
    }

    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

private:
    Logger() : minLevel(LogLevel::DEBUG) {}
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& message) {
        if (level >= minLevel) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::string levelStr;
            switch (level) {
                case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
                case LogLevel::INFO:    levelStr = "INFO"; break;
                case LogLevel::WARNING: levelStr = "WARNING"; break;
                case LogLevel::ERROR:   levelStr = "ERROR"; break;
            }
            std::cout << "[" << levelStr << "] " << message << std::endl;
        }
    }

    LogLevel minLevel;
    std::mutex logMutex;
};

#define LOG_DEBUG(msg)   Logger::getInstance().debug(msg)
#define LOG_INFO(msg)    Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg)   Logger::getInstance().error(msg)
