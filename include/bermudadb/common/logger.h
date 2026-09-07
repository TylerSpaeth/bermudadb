#ifndef BERMUDADB_LOGGER_H
#define BERMUDADB_LOGGER_H

#include <mutex>
#include <string>

namespace bermudadb {

    class Logger {

    public:

        /// @brief A logger that logs to the given filepath. Note that in debug builds this will
        /// also duplicate all logs to stdout.
        explicit Logger(const char* filepath);

        ~Logger();

        /// @brief Writes a log at the debug level.
        /// @param message - The message to log
        void Info(const std::string& message) const;

        /// @brief Writes a log at the warning level.
        /// @param message - The message to log
        void Warning(const std::string& message) const;

        /// @brief Writes a log at the error level.
        /// @param message - The message to log
        void Error(const std::string& message) const;

        /// @brief Writes a log at the debug level.
        /// @param message - The message to log
        void Debug(const std::string& message) const;

    private:

        FILE* m_LogFile;
        static inline const std::string INFO = "   Info";
        static inline const std::string WARNING = "Warning";
        static inline const std::string ERROR = "  Error";
        static inline const std::string DEBUG = "  Debug";

        static inline std::mutex s_Mutex;

        /// @brief Writes a message with the with a log type and timestamp to the logfile.
        /// @param logTypeString - Represents the type of log this is.
        /// @param message - The actual message of the log
        void WriteLog(const std::string& logTypeString, const std::string& message) const;

    };

}

#endif //BERMUDADB_LOGGER_H