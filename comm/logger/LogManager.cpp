//
// Created by haichao.zhang on 2022/5/3.
//

#include "LogManager.h"
#include <map>
#include <mutex>
#include "spdlog/spdlog.h"

std::map<string, std::shared_ptr<IBaseLogger>> logger_map_;

level_enum defaut_level_ = level_enum::info;

string default_pattern_ = "[%Y-%m-%d %H:%M:%S.%e][%l]%v [tid:%t][pid:%P]";

std::string default_logger_name_ = "Default";

string default_log_file_ = "default.log";

std::recursive_mutex mutex_;

bool default_print_console_ = true;

void LogManager::SetDefaultLevel(const std::string &level)
{
    if ("TRACE" == level)
    {
        SetDefaultLevel(level_enum::trace);
    }
    else if ("DEBUG" == level)
    {
        SetDefaultLevel(level_enum::debug);
    }
    else if ("INFO" == level)
    {
        SetDefaultLevel(level_enum::info);
    }
    else if ("WARN" == level)
    {
        SetDefaultLevel(level_enum::warn);
    }
    else if ("ERROR" == level)
    {
        SetDefaultLevel(level_enum::error);
    }
    else if ("CRITICAL" == level)
    {
        SetDefaultLevel(level_enum::critical);
    }
    else if ("OFF" == level)
    {
        SetDefaultLevel(level_enum::off);
    }
    else
    {
        char error[256]{};
        sprintf(error, "undefined log level %s, only allowed levels(case sensitive): TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF", level.c_str());
        throw std::runtime_error(error);
    }
}

void LogManager::SetDefaultLevel(const level_enum &level)
{
    std::unique_lock<std::recursive_mutex> lk(mutex_);
    defaut_level_ = level;
}

void LogManager::SetDefaultPattern(const std::string &pattern)
{
    std::unique_lock<std::recursive_mutex> lk(mutex_);
    default_pattern_ = pattern;
}

void LogManager::SetDefaultLoggerName(const std::string &name)
{
    std::unique_lock<std::recursive_mutex> lk(mutex_);
    default_logger_name_ = name;
}

void LogManager::SetDefaultConsole(bool print_console)
{
    std::unique_lock<std::recursive_mutex> lk(mutex_);
    default_print_console_ = print_console;
}

void LogManager::SetDefaultLogFile(const std::string &log_file)
{
    std::unique_lock<std::recursive_mutex> lk(mutex_);
    default_log_file_ = log_file;
}

std::shared_ptr<IBaseLogger>LogManager::GetLogger()
{
    return GetLogger(default_logger_name_, default_log_file_, defaut_level_, default_pattern_, default_print_console_);
}

std::shared_ptr<IBaseLogger> LogManager::GetLogger(const std::string &name)
{
    return GetLogger(name, default_log_file_, defaut_level_, default_pattern_, default_print_console_);
}

std::shared_ptr<IBaseLogger>
LogManager::GetLogger(const std::string &name, const std::string &log_file)
{
    return GetLogger(name, log_file, defaut_level_, default_pattern_, default_print_console_);
}

std::shared_ptr<IBaseLogger>
LogManager::GetLogger(const std::string &name, const std::string &log_file, level_enum level)
{
    return GetLogger(name, log_file, level, default_pattern_, default_print_console_);
}

std::shared_ptr<IBaseLogger>
LogManager::GetLogger(const std::string &name, const std::string &log_file, level_enum level, const std::string &pattern)
{
    return GetLogger(name, log_file, level, pattern, default_print_console_);
}

std::shared_ptr<IBaseLogger>
LogManager::GetLogger(const std::string &name, const std::string &log_file, level_enum level, const std::string &pattern, bool print_console)
{
    std::unique_lock<std::recursive_mutex> lk(mutex_);
    if (logger_map_.find(name) == logger_map_.end())
    {
        std::shared_ptr<IBaseLogger> logger = IBaseLogger::CreateInstance(name, log_file, level, pattern, print_console);
        logger->Initialize();
        logger_map_[name] = logger;
    }
    return logger_map_[name];
}

void LogManager::DropAll()
{
    logger_map_.clear();
}

LogManager &LogManager::instance()
{
    static LogManager s_instance;
    return s_instance;
}

LogManager::~LogManager()
{
    spdlog::drop_all();
}