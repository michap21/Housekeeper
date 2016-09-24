#pragma once

#include <sstream>
#include <memory>
#include <string>

#define _P_LOG_INFO    ::log::internal::LogMessage(log::INFO)
#define _P_LOG_WARNING ::log::internal::LogMessage(log::WARNING)
#define _P_LOG_ERROR   ::log::internal::LogMessage(log::ERROR)
#define _P_LOG_FATAL   ::log::internal::LogMessageFatal()

#define P_LOG(severity) _P_LOG_##severity

/**
 * Generate Unique Variable Name via macro.
 */
#define UNIQUE_NAME(base) base##__LINE__

#define P_LOG_FIRST_N(severity, n)                                          \
    static int UNIQUE_NAME(LOG_OCCURRENCES) = 0;                            \
    if (UNIQUE_NAME(LOG_OCCURRENCES) <= n) ++UNIQUE_NAME(LOG_OCCURRENCES);  \
    if (UNIQUE_NAME(LOG_OCCURRENCES) <= n) P_LOG(severity)

#define P_LOG_IF_EVERY_N(severity, condition, n)                            \
    static int UNIQUE_NAME(LOG_OCCURRENCES) = 0;                            \
    if (condition && ((UNIQUE_NAME(LOG_OCCURRENCES) =                       \
    (UNIQUE_NAME(LOG_OCCURRENCES) + 1) % n) == (1 % n)))                    \
    P_LOG(severity)

#define P_LOG_EVERY_N(severity, n) P_LOG_IF_EVERY_N(severity, true, n)

// TODO(jeff): Define a proper implementation of VLOG_IS_ON
#define P_VLOG_IS_ON(lvl)       ((lvl) <= 0)

#define P_LOG_IF(severity, condition) \
    if (condition) P_LOG(severity)

#define P_VLOG(lvl)             P_LOG_IF(INFO, P_VLOG_IS_ON(lvl))
#define P_VLOG_IF(lvl, cond)    P_LOG_IF(INFO, P_VLOG_IS_ON(lvl) && cond)
#define P_VLOG_EVERY_N(lvl, n)  P_LOG_IF_EVERY_N(INFO, P_VLOG_IS_ON(lvl), n)

#define PREDICT_FALSE(x)        (__builtin_expect(x, 0))
#define PREDICT_TRUE(x)         (__builtin_expect(!!(x), 1))

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK(fp->Write(x) == 4)
#define P_CHECK(condition)                              \
    if (PREDICT_FALSE(!(condition)))                    \
        P_LOG(FATAL) << "Check failed: " #condition " "

#define P_CHECK_EQ(val1, val2)  P_CHECK((val1) == (val2))
#define P_CHECK_NE(val1, val2)  P_CHECK((val1) != (val2))
#define P_CHECK_LE(val1, val2)  P_CHECK((val1) <= (val2))
#define P_CHECK_LT(val1, val2)  P_CHECK((val1) < (val2))
#define P_CHECK_GE(val1, val2)  P_CHECK((val1) >= (val2))
#define P_CHECK_GT(val1, val2)  P_CHECK((val1) > (val2))
#define P_CHECK_NOTNULL(val)    P_CHECK((val) != nullptr)

//! GLOG compatible APIs
//! NOTE: only implement frequently used APIs.
#define LOG(x)                  P_LOG(x)
#define VLOG(x)                 P_VLOG(x)
#define CHECK(x)                P_CHECK(x)
#define PCHECK(x)               P_CHECK(x)

#define CHECK_EQ(val1, val2)    P_CHECK((val1) == (val2))
#define CHECK_NE(val1, val2)    P_CHECK((val1) != (val2))
#define CHECK_LE(val1, val2)    P_CHECK((val1) <= (val2))
#define CHECK_LT(val1, val2)    P_CHECK((val1) < (val2))
#define CHECK_GE(val1, val2)    P_CHECK((val1) >= (val2))
#define CHECK_GT(val1, val2)    P_CHECK((val1) > (val2))
#define CHECK_NOTNULL(val)      P_CHECK((val) != nullptr)

#define VLOG_IS_ON(x)               P_VLOG_IS_ON(x)
#define VLOG_IF(lvl, cond)          P_VLOG_IF(lvl, cond)
#define VLOG_EVERY_N(lvl, n)        P_VLOG_EVERY_N(lvl, n)

#define LOG_FIRST_N(severity, n)    P_LOG_FIRST_N(severity, n)
#define LOG_IF(severity, condition) P_LOG_IF(severity, condition)
#define LOG_EVERY_N(severity, n)    P_LOG_EVERY_N(severity, n)

namespace log {

    //! Log levels.
    const int INFO = 0;
    const int WARNING = 1;
    const int ERROR = 2;
    const int FATAL = 3;
    const int NUM_SEVERITIES = 4;

    namespace internal {

        class LogMessage : public std::basic_ostringstream<char> {
        public:
            LogMessage(int severity, const char* fname = __FILE__, int line = __LINE__);
            ~LogMessage();

        protected:
            /**
             * @brief Print log message to stderr, files, etc.
             */
            void generateLogMessage();

        private:
            const char* fname_;
            int line_;
            int severity_;
        };

        // LogMessageFatal ensures the process will exit in failure after
        // logging this message.
        class LogMessageFatal : public LogMessage {
        public:
            LogMessageFatal(const char* file = __FILE__, int line = __LINE__) __attribute__((cold));
            ~LogMessageFatal() __attribute__((noreturn));
        };

    }  //  namespace internal

    /**
     * @brief initialize logging
     * @note: Current implement of logging is lack of:
     *          PrintCallStack when fatal.
     *          VLOG_IS_ON
     *        But it is portable to multi-platform, and simple enough to modify.
     */
    void initializeLogging(int argc, const char** argv);

    namespace logging {
        /**
         * @brief Set Min Log Level. if Log.level < minLogLevel, then will not print log
         *        to stream
         * @param level. Any integer is OK, but only 0 <= x <= NUM_SEVERITIES is useful.
         */
        void setMinLogLevel(int level);

        /**
         * @brief Install Log(Fatal) failure function. Default is abort();
         * @param callback: The failure function.
         */
        void installFailureFunction(void (*callback)());

        /**
         * @brief installFailureWriter
         * @note: not implemented currently.
         */
        inline void installFailureWriter(void(*callback)(const char*, int)) {
            (void)(callback);  // unused callback.
        }
    }  //  namespace logging
}  //  namespace log
