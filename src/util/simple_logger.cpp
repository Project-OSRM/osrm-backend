#include "util/simple_logger.hpp"
#ifdef _MSC_VER
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>

namespace
{
static const char COL_RESET[]{"\x1b[0m"};
static const char RED[]{"\x1b[31m"};
#ifndef NDEBUG
static const char YELLOW[]{"\x1b[33m"};
#endif
// static const char GREEN[] { "\x1b[32m"};
// static const char BLUE[] { "\x1b[34m"};
// static const char MAGENTA[] { "\x1b[35m"};
// static const char CYAN[] { "\x1b[36m"};
}

void LogPolicy::Unmute() { m_is_mute = false; }

void LogPolicy::Mute() { m_is_mute = true; }

bool LogPolicy::IsMute() const { return m_is_mute; }

LogPolicy &LogPolicy::GetInstance()
{
    static LogPolicy runningInstance;
    return runningInstance;
}

SimpleLogger::SimpleLogger() : level(logINFO) {}

std::mutex &SimpleLogger::get_mutex()
{
    static std::mutex mtx;
    return mtx;
}

std::ostringstream &SimpleLogger::Write(LogLevel lvl) noexcept
{
    std::lock_guard<std::mutex> lock(get_mutex());
    level = lvl;
    os << "[";
    switch (level)
    {
    case logWARNING:
        os << "warn";
        break;
    case logDEBUG:
#ifndef NDEBUG
        os << "debug";
#endif
        break;
    default: // logINFO:
        os << "info";
        break;
    }
    os << "] ";
    return os;
}

SimpleLogger::~SimpleLogger()
{
    std::lock_guard<std::mutex> lock(get_mutex());
    if (!LogPolicy::GetInstance().IsMute())
    {
        const bool is_terminal = static_cast<bool>(isatty(fileno(stdout)));
        switch (level)
        {
        case logWARNING:
            std::cerr << (is_terminal ? RED : "") << os.str() << (is_terminal ? COL_RESET : "")
                      << std::endl;
            break;
        case logDEBUG:
#ifndef NDEBUG
            std::cout << (is_terminal ? YELLOW : "") << os.str() << (is_terminal ? COL_RESET : "")
                      << std::endl;
#endif
            break;
        case logINFO:
        default:
            std::cout << os.str() << (is_terminal ? COL_RESET : "") << std::endl;
            break;
        }
    }
}
