/*
*/
#ifndef TRADEBEE_IBASE_LOGGER_H
#define TRADEBEE_IBASE_LOGGER_H

#include <string>
#include <memory>


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#define __FILENAME__ std::string(strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ std::string(strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

using namespace std;

#define LOG_STR_H(x) #x
#define LOG_STR_HELPER(x) LOG_STR_H(x)
#define LOG_TRACE(logger, ...) logger->trace("[" + __FILENAME__ + ":" + LOG_STR_HELPER(__LINE__) + "] " + __VA_ARGS__)
#define LOG_DEBUG(logger, ...) logger->debug("[" + __FILENAME__ + ":" + LOG_STR_HELPER(__LINE__) + "] " + __VA_ARGS__)
#define LOG_INFO(logger, ...) logger->info("[" + __FILENAME__ + ":" + LOG_STR_HELPER(__LINE__) + "] " + __VA_ARGS__)
#define LOG_WARN(logger, ...) logger->warn("[" + __FILENAME__ + ":" + LOG_STR_HELPER(__LINE__) + "] " +__VA_ARGS__)
#define LOG_ERROR(logger, ...) logger->error("[" + __FILENAME__ + ":" + LOG_STR_HELPER(__LINE__) + "] " + __VA_ARGS__)
#define LOG_CRITICAL(logger, ...) logger->critical("[" + __FILENAME__ + ":" + LOG_STR_HELPER(__LINE__) + "] " + __VA_ARGS__)
#define LOG_OFF(logger, ...) logger->off("[" + __FILENAME__ + ":" + LOG_STR_HELPER(__LINE__) + "] " + __VA_ARGS__)


        enum class level_enum
        {
            trace = 0,
            debug = 1,
            info = 2,
            warn = 3,
            error = 4,
            critical = 5,
            off = 6
        } ;


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        class __declspec(dllexport) IBaseLogger {
#else

        class IBaseLogger
        {
#endif
        public:

            static std::shared_ptr<IBaseLogger> CreateInstance(const string &name, const string &log_file_, level_enum level, const string &pattern_, bool print_to_console_);

            template<typename... Args>
            void log(level_enum lvl, const string &fmt, const Args &... args);

            template<typename T>
            void log(level_enum lvl, const T &);

            template<typename... Args>
            void trace(const string &fmt, const Args &... args);

            template<typename... Args>
            void debug(const string &fmt, const Args &... args);

            template<typename... Args>
            void info(const string &fmt, const Args &... args);

            template<typename... Args>
            void warn(const string &fmt, const Args &... args);

            template<typename... Args>
            void error(const string &fmt, const Args &... args);

            template<typename... Args>
            void critical(const string &fmt, const Args &... args);


            template<typename T>
            void trace(const T &);

            template<typename T>
            void debug(const T &);

            template<typename T>
            void info(const T &);

            template<typename T>
            void warn(const T &);

            template<typename T>
            void error(const T &);

            template<typename T>
            void critical(const T &);

            virtual void SetLevel(level_enum level) = 0;

            virtual void SetPattern(const string &pattern) = 0;

            virtual void Initialize() = 0;

        protected:
            virtual void logImpl(level_enum lvl, const string &msg) = 0;
        };


#endif //IBASE_LOGGER_H

#include "IBaseLoggerImpl.h"