

#ifndef TRADEBEE_BASELOGGERSPDIMPL_H
#define TRADEBEE_BASELOGGERSPDIMPL_H


#include "IBaseLogger.h"
#include "spdlog/spdlog.h"


        class BaseLoggerSpdImpl : public IBaseLogger
        {
        private:
            std::shared_ptr<spdlog::logger> spdlog_;
            std::string log_name_;
            std::string log_file_;
            std::string pattern_;
            level_enum level_;
            bool print_console_;
        public:
            explicit BaseLoggerSpdImpl(const string &log_name, const string &log_file, level_enum level, const string &pattern, bool print_console);

            ~BaseLoggerSpdImpl() = default;

            void logImpl(level_enum lvl, const string &msg) override;

            void SetLevel(level_enum level) override;

            void SetPattern(const string &pattern) override;

            void Initialize() override;

        };

#endif
