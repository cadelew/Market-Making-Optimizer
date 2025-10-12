#pragma once
#include <string>
#include <iostream>
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
        // Simple logging - just use std::cout for now
        void log_with_timestamp(const std::string& level, const std::string& message);
    };
}