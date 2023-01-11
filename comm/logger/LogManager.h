//
// Created by haichao.zhang on 2022/5/3.
//

#ifndef TRADEBEE_LOGMANAGER_H
#define TRADEBEE_LOGMANAGER_H

#include "IBaseLogger.h"

class LogManager {

private:
    ~LogManager();

    LogManager() = default;

public:
    LogManager(LogManager const &) = delete;

    void operator=(LogManager const &)= delete;

public:
    static LogManager &instance();

    static void SetDefaultLevel(const std::string &level);

    static void SetDefaultLevel(const level_enum &level);

    static void SetDefaultPattern(const std::string &pattern);

    static void SetDefaultLoggerName(const std::string &name);

    static void SetDefaultLogFile(const std::string &log_file);

    static void SetDefaultConsole(bool print_to_console);

    static shared_ptr<IBaseLogger> GetLogger(const std::string &name);

    static shared_ptr<IBaseLogger> GetLogger(const std::string &name, const std::string &log_file);

    static shared_ptr<IBaseLogger> GetLogger(const std::string &name, const std::string &log_file, level_enum level);

    static shared_ptr<IBaseLogger> GetLogger(const std::string &name, const std::string &log_file, level_enum level, const std::string &pattern);

    static shared_ptr<IBaseLogger> GetLogger(const std::string &name, const std::string &log_file, level_enum level, const std::string &pattern, bool print_to_console);

    static shared_ptr<IBaseLogger> GetLogger();

    static void DropAll();
};


#endif //TRADEBEE_LOGMANAGER_H
