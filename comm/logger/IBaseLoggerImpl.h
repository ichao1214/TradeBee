
#ifndef IBASE_LOGGER_CPP
#define IBASE_LOGGER_CPP


#include "IBaseLogger.h"
#include <spdlog/fmt/fmt.h>


using namespace std;


        template<typename... Args>
        inline void IBaseLogger::trace(const string &fmt, const Args &... args)
        {
            this->log(level_enum::trace, fmt, args...);
        }

        template<typename... Args>
        inline void IBaseLogger::debug(const string &fmt, const Args &... args)
        {
            this->log(level_enum::debug, fmt, args...);
        }

        template<typename... Args>
        inline void IBaseLogger::info(const string &fmt, const Args &... args)
        {
            this->log(level_enum::info, fmt, args...);
        }

        template<typename... Args>
        inline void IBaseLogger::warn(const string &fmt, const Args &... args)
        {
            this->log(level_enum::warn, fmt, args...);
        }

        template<typename... Args>
        inline void IBaseLogger::error(const string &fmt, const Args &... args)
        {
            this->log(level_enum::error, fmt, args...);
        }

        template<typename... Args>
        inline void IBaseLogger::critical(const string &fmt, const Args &... args)
        {
            this->log(level_enum::critical, fmt, args...);
        }

        template<typename... Args>
        inline void IBaseLogger::log(level_enum lvl, const string &fmt, const Args &... args)
        {
            this->logImpl(lvl, fmt::format(fmt, args...));
        }

        template<typename T>
        inline void IBaseLogger::log(level_enum lvl, const T &value)
        {
            this->log(lvl, "{0}", value);
        }

        template<typename T>
        inline void IBaseLogger::trace(const T &msg)
        {
            this->log(level_enum::trace, msg);
        }

        template<typename T>
        inline void IBaseLogger::debug(const T &msg)
        {
            this->log(level_enum::debug, msg);
        }

        template<typename T>
        inline void IBaseLogger::info(const T &msg)
        {
            this->log(level_enum::info, msg);
        }

        template<typename T>
        inline void IBaseLogger::warn(const T &msg)
        {
            this->log(level_enum::warn, msg);
        }

        template<typename T>
        inline void IBaseLogger::error(const T &msg)
        {
            this->log(level_enum::error, msg);
        }

        template<typename T>
        inline void IBaseLogger::critical(const T &msg)
        {
            this->log(level_enum::critical, msg);
        }



#endif