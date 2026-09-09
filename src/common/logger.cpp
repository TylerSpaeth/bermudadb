#include <chrono>
#include <cstring>
#include <bermudadb/common/logger.hpp>

#include <iostream>
#include <print>

namespace bermudadb {

    Logger::Logger(const char* filepath) {
        m_LogFile = fopen(filepath, "w");
        if (!m_LogFile) {
            std::println(stderr,
             "Failed to open log file for writing: [{}] {}",
             filepath ? filepath : "null",
             std::strerror(errno));
            m_LogFile = stdout;
        }
    }

    Logger::~Logger() {
        if (m_LogFile && m_LogFile != stdout) {
            fclose(m_LogFile);
        }
    }

    void Logger::info(const std::string& message) const {
        write_log(INFO, message);
    }
    void Logger::warning(const std::string& message) const {
        write_log(WARNING, message);
    }
    void Logger::error(const std::string& message) const {
        write_log(ERROR, message);
    }
    void Logger::debug(const std::string& message) const {
        write_log(DEBUG, message);
    }

    void Logger::write_log(const std::string& logTypeString, const std::string& message) const {
        std::lock_guard<std::mutex> lock(s_Mutex);

        auto timestamp = std::chrono::system_clock::now();
        std::println(m_LogFile, "{:%Y-%m-%d %H:%M:%S} {} : {}", timestamp, logTypeString, message);
        fflush(m_LogFile);


        if (m_LogFile != stdout) {
            std::println(stdout, "Logged to file: {:%Y-%m-%d %H:%M:%S} {} : {}", timestamp, logTypeString, message);
        }
    }

}