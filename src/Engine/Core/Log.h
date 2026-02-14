#pragma once

#include <string>

namespace rg
{
enum class LogLevel
{
    Info,
    Warning,
    Error
};

class Log
{
public:
    static void Write(LogLevel level, const std::string& message);
};
} // namespace rg
