#pragma once
#include <spdlog/spdlog.h>
#include <memory>
#include <string>
#include "Quote.h"
#include "Fill.h"

namespace mm {
    class Logger {
    public:
        Logger();
        
        void log_quote_placed(const Quote& quote);
        void log_fill(const Fill& fill);
        void log_info(const std::string& message);
        void log_error(const std::string& message);
        
    private:
        std::shared_ptr<spdlog::logger> trade_logger_;
        std::shared_ptr<spdlog::logger> general_logger_;
    };
}