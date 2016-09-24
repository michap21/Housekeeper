#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"

namespace log {

    namespace internal {

        std::string join(const std::string& part1, const std::string& part2) {
            const char sep = '/';
            if (!part2.empty() && part2.front() == sep) {
                return part2;
            }
            std::string ret;
            ret.reserve(part1.size() + part2.size() + 1);
            ret = part1;
            if (!ret.empty() && ret.back() != sep) {
                ret += sep;
            }
            ret += part2;
            return ret;
        }

        static inline bool env2bool(const char* envName, bool defaultValue = false) {
            char* envValue = getenv(envName);
            if (envValue == nullptr) {
                return defaultValue;
            } else {
                return memchr("tTyY1\0", envValue[0], 6) != nullptr;
            }
        }

        static inline int env2int(const char* envName, int defaultValue = 0) {
            char* envValue = getenv(envName);
            if (envValue == nullptr) {
                return defaultValue;
            } else {
                int retValue = defaultValue;
                try {
                    retValue = std::stoi(envValue);
                } catch (...) {
                    // pass
                }
                return retValue;
            }
        }

        static inline int env2index(const char* envName,
                                    const std::vector<std::string>& options,
                                    int defaultValue) {
            char* envValue = getenv(envName);
            if (envValue == nullptr) {
                return defaultValue;
            } else {
                for (size_t i = 0; i < options.size(); ++i) {
                    if (options[i] == envValue) {
                        return static_cast<int>(i);
                    }
                }
                return defaultValue;
            }
        }

        static bool gLogToStderr = env2bool("PLOG_LOGTOSTDERR", true);
        static const std::vector<std::string> gLevelName = {"INFO", "WARNING", "ERROR",
            "FATAL"};
        static int gMinLogLevel =
        env2int("PLOG_MINLOGLEVEL", env2index("PLOG_MINLOGLEVEL", gLevelName, 0));

        static std::vector<std::vector<int>> gLogFds;
        static std::vector<int> gLogFileFds;
        static bool gLogInited = false;
        static void freeLogFileFds() {
            for (auto fd : gLogFileFds) {
                close(fd);
            }
        }

        static void initializeLogFds(const char* argv0) {
            gLogFds.resize(NUM_SEVERITIES);

            for (int i = gMinLogLevel; i < NUM_SEVERITIES && gLogToStderr;
                 ++i) {  // Add stderr
                std::vector<int>& fds = gLogFds[i];
                fds.push_back(STDERR_FILENO);
            }

            char* logDir = getenv("PLOG_LOGDIR");

            for (int i = gMinLogLevel; i < NUM_SEVERITIES && logDir != nullptr; ++i) {
                std::string filename =
                join(logDir, std::string(argv0) + "." + gLevelName[i]);
                int fd = open(filename.c_str(), O_CREAT | O_WRONLY, 0644);
                if (fd == -1) {
                    fprintf(stderr, "Open log file error!");
                    exit(1);
                }
                gLogFileFds.push_back(fd);

                std::vector<int>& curFds = gLogFds[i];
                curFds.insert(curFds.end(), gLogFileFds.begin(), gLogFileFds.end());
            }

            atexit(freeLogFileFds);
            gLogInited = true;
        }

        static void (*gFailureFunctionPtr)() __attribute__((noreturn)) = abort;

        LogMessage::LogMessage(int severity, const char* fname, int line)
        : fname_(fname), line_(line), severity_(severity) {}

        LogMessage::~LogMessage() { this->generateLogMessage(); }

        void LogMessage::generateLogMessage() {
            if (!gLogInited) {
                fprintf(stderr, "%c %s:%d] %s\n", "IWEF"[severity_], fname_, line_,
                        str().c_str());
            } else {
                for (auto& fd : gLogFds[this->severity_]) {
                    dprintf(fd, "%c %s:%d] %s\n", "IWEF"[severity_], fname_, line_,
                            str().c_str());
                }
            }
        }

        LogMessageFatal::LogMessageFatal(const char* file, int line)
        : LogMessage(FATAL, file, line) {}

        LogMessageFatal::~LogMessageFatal() {
            generateLogMessage();
            gFailureFunctionPtr();
        }
    }  // namespace internal

    void initializeLogging(int argc, const char** argv) {
        internal::initializeLogFds(argv[0]);
    }

    namespace logging {
        void setMinLogLevel(int level) {
            log::internal::gMinLogLevel = level;
        }

        void installFailureFunction(void (*callback)()__attribute__((noreturn)) = abort) {
            log::internal::gFailureFunctionPtr = callback;
        }

    }  // namespace logging

}  // namespace log