#include "Engine/Core/Log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace rg
{
namespace
{
const char* ToString(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "LOG";
    }
}
} // namespace

void Log::Write(const LogLevel level, const std::string& message)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream oss;
    oss << "[" << std::put_time(&localTime, "%H:%M:%S") << "] "
        << "[" << ToString(level) << "] "
        << message;

    std::cout << oss.str() << '\n';
}
} // namespace rg
